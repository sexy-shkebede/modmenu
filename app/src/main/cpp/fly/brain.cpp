#include "brain.hpp"

#include <math.h>

namespace
{
    constexpr int    kPowSteps = 1 << 16;   // таблица степеней: 6.5 с при dt 0.1 мс; дальше ток и отклонение — ноль
    constexpr double kMargin   = 1e-3;      // мВ: запас до порога, с которым нейрон считаем шаг за шагом

    uint64_t Mix(uint64_t z)
    {
        z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ull;
        z = (z ^ (z >> 27)) * 0x94D049BB133111EBull;
        return z ^ (z >> 31);
    }
}

fly::SBrainParams fly::OriginalModel()
{
    SBrainParams p;
    p.a_inc = 0.f;
    p.U     = 0.f;
    return p;
}

fly::CBrain::CBrain(const SConnectome& c, const SBrainParams& p, uint64_t uSeed, bool bDense)
    : m_C(c), m_P(p), m_uSeed(uSeed), m_bDense(bDense)
{
    // Константы float32 — как их передаёт в ядро brain.py (_set_consts)
    m_nDelay  = (int)lround(p.t_dly / p.dt);
    m_nRfc    = (int)lround(p.t_rfc / p.dt);
    if (m_nDelay < 1) m_nDelay = 1;
    if (m_nDelay > 64) m_nDelay = 64;
    m_bUseA   = p.a_inc > 0.f;
    m_bUseU   = p.U > 0.f;
    m_cMbr    = (float)((double)p.dt / p.t_mbr);
    m_cDecG   = (float)exp(-(double)p.dt / p.tau);
    m_cDecA   = (float)exp(-(double)p.dt / p.tau_a);
    m_cD      = (float)((double)p.dt / p.tau_d);
    m_cKeepX  = (float)(1.0 - p.U);
    m_cKick   = (float)((double)p.w_syn * p.f_poi);
    m_cPScale = (float)((double)p.dt * 1e-3);
    m_dLogDecA = log((double)m_cDecA);
    m_dLog1mD  = log(1.0 - (double)m_cD);
    for (int k = 0; k < (1 << 14); k++) m_aWeight[k] = (float)((double)(k - (1 << 13)) * p.w_syn);

    // Шаг Эйлера без входов: u' = beta·u + c·(g − a), g' = gamma·g, a' = alpha·a (u = v − v_0)
    m_dBeta  = 1.0 - (double)m_cMbr;
    m_dGamma = (double)m_cDecG;
    m_dAlpha = m_bUseA ? (double)m_cDecA : 1.0;
    m_dLogBeta  = log(m_dBeta);
    m_dLogGamma = log(m_dGamma);
    m_PowBeta.resize(kPowSteps); m_PowGamma.resize(kPowSteps); m_PowAlpha.resize(kPowSteps); m_PowRec.resize(kPowSteps);
    m_dPsp = 0.0;
    for (int k = 0; k < kPowSteps; k++)
    {
        const double bk = exp((double)k * m_dLogBeta), gk = exp((double)k * m_dLogGamma);
        m_PowBeta[k]  = (float)bk;
        m_PowGamma[k] = (float)gk;
        m_PowAlpha[k] = (float)exp((double)k * m_dLogDecA);
        m_PowRec[k]   = (float)exp((double)k * m_dLog1mD);
        const double psp = (double)m_cMbr * (bk - gk) / (m_dBeta - m_dGamma);
        if (psp > m_dPsp) m_dPsp = psp;
    }
    m_dPsp *= 1.0001;   // с запасом на округление

    const size_t n = c.N;
    m_V.resize(n); m_G.resize(n); m_A.resize(n); m_X.resize(n); m_Rate.resize(n);
    m_Rfc.resize(n); m_Cnt.resize(n); m_Slept.resize(n); m_Awake.resize(n); m_Sil.assign(n, 0);
    m_In.assign(n, 0.f); m_NextIn.assign(n, UINT64_MAX);
    m_Active.reserve(n);
    Reset();
}

void fly::CBrain::Reset()
{
    const size_t n = m_C.N;
    for (size_t i = 0; i < n; i++)
    {
        m_V[i] = m_P.v_0; m_G[i] = 0.f; m_A[i] = 0.f; m_X[i] = 1.f; m_Rate[i] = 0.f; m_In[i] = 0.f;
        m_Rfc[i] = 0; m_Cnt[i] = 0; m_Slept[i] = m_uStep; m_Awake[i] = m_bDense ? 1 : 0; m_NextIn[i] = UINT64_MAX;
    }
    m_Active.clear();
    m_Touched.clear();
    for (std::vector<uint32_t>& t : m_Timer) t.clear();
    if (m_bDense)
        for (uint32_t i = 0; i < (uint32_t)n; i++) m_Active.push_back(i);
    for (std::vector<SSpike>& q : m_Ring) q.clear();
    m_SpikeLog.clear();
    // m_uStep не сбрасываем: генератор шума идёт дальше, как в brain.py
}

void fly::CBrain::SetRate(uint32_t i, float fHz)
{
    if (i >= m_C.N) return;
    const float fOld = m_Rate[i];
    m_Rate[i] = fHz > 0.f ? fHz : 0.f;
    if (m_Rate[i] != fOld) NextInput(i, m_uStep);   // поток без памяти: новую частоту можно начать с любого шага
}

void fly::CBrain::NextInput(uint32_t i, uint64_t uFrom)
{
    // В brain.py вход — независимое "да/нет" на каждом шаге с вероятностью p = rate·dt.
    // То же самое распределение: промежуток до следующего "да" — геометрический.
    const double p = (double)(m_Rate[i] * m_cPScale);
    if (p <= 0.0) { m_NextIn[i] = UINT64_MAX; return; }
    uint64_t gap = 0;
    if (p < 1.0)
    {
        const double u = Uniform(i, uFrom);
        const double g = floor(log(u) / log1p(-p));
        gap = g > 1e12 ? (uint64_t)1e12 : (uint64_t)g;
    }
    m_NextIn[i] = uFrom + gap;
    if (!m_Awake[i]) m_Timer[m_NextIn[i] & ((1u << 12) - 1)].push_back(i);
}

void fly::CBrain::Silence(uint32_t i, bool bSilent)
{
    if (i < m_C.N) m_Sil[i] = bSilent ? 1 : 0;
}

double fly::CBrain::Uniform(uint32_t i, uint64_t uSalt) const
{
    // Счётчиковый генератор: число зависит только от (зерно, шаг, нейрон) — не от порядка счёта. (0, 1]
    const uint64_t r = Mix(m_uSeed ^ Mix(uSalt * 0x9E3779B97F4A7C15ull + i));
    return ((double)(r >> 11) + 1.0) * (1.0 / 9007199254740992.0);
}

double fly::CBrain::Pow(const std::vector<float>& t, double dLog, uint64_t k) const
{
    return k < (uint64_t)kPowSteps ? (double)t[k] : exp((double)k * dLog);
}

void fly::CBrain::Advance(uint32_t i)
{
    // k шагов без входов — по формуле (u = v − v_0):
    //   u_k = β^k·u + c·g·(β^k − γ^k)/(β − γ) − c·a·(β^k − α^k)/(β − α),  g_k = γ^k·g,  a_k = α^k·a
    const uint64_t k = m_uStep - m_Slept[i];
    if (k == 0) return;
    const double c  = (double)m_cMbr;
    const double bk = k < (uint64_t)kPowSteps ? (double)m_PowBeta[k] : 0.0;
    const double gk = k < (uint64_t)kPowSteps ? (double)m_PowGamma[k] : 0.0;
    const double u  = (double)m_V[i] - (double)m_P.v_0, g = m_G[i], a = m_A[i];
    double uk = bk * u + c * g * (bk - gk) / (m_dBeta - m_dGamma);
    if (a != 0.0)
    {
        const double ak = Pow(m_PowAlpha, m_dLogDecA, k);
        uk -= c * a * (bk - ak) / (m_dBeta - m_dAlpha);
        m_A[i] = (float)(a * ak);
    }
    m_V[i] = (float)((double)m_P.v_0 + uk);
    m_G[i] = (float)(g * gk);
    if (m_bUseU && m_X[i] != 1.f) m_X[i] = (float)(1.0 - (1.0 - (double)m_X[i]) * Pow(m_PowRec, m_dLog1mD, k));
    m_Slept[i] = m_uStep;
}

bool fly::CBrain::MaySpike(float v, float g, float a) const
{
    // Без новых входов: u_k = P·β^k − Q·γ^k + R·α^k. Ток g ≤ 0 мембрану не поднимает.
    if (g <= 0.f) return false;
    const double u   = (double)v - (double)m_P.v_0;
    const double thr = (double)(m_P.v_th - m_P.v_0) - kMargin;
    if ((u > 0.0 ? u : 0.0) + m_dPsp * (double)g < thr) return false;   // даже пик ПСП не дотянет (адаптация только опускает)
    const double c = (double)m_cMbr, b = m_dBeta, gm = m_dGamma;
    const double Q = c * (double)g / (b - gm);
    const double R = a != 0.f ? c * (double)a / (b - m_dAlpha) : 0.0;
    const double P = u + Q - R;
    const auto U = [&](double k)
    {
        const uint64_t n = (uint64_t)k;
        const double bk = n < (uint64_t)kPowSteps ? (double)m_PowBeta[n] : 0.0;
        const double gk = n < (uint64_t)kPowSteps ? (double)m_PowGamma[n] : 0.0;
        return P * bk - Q * gk + (R != 0.0 ? R * Pow(m_PowAlpha, m_dLogDecA, n) : 0.0);
    };
    // максимум P·β^k − Q·γ^k: (β/γ)^k = Q·ln γ / (P·ln β); α^k почти постоянна и только опускает
    double kMax = 1.0;
    if (P > 0.0)
    {
        const double r = (Q * m_dLogGamma) / (P * m_dLogBeta);
        if (r > 1.0) kMax = log(r) / (m_dLogBeta - m_dLogGamma);
    }
    const double k0 = floor(kMax) < 1.0 ? 1.0 : floor(kMax);
    return U(k0) > thr || U(k0 + 1.0) > thr || U(1.0) > thr;
}

void fly::CBrain::Wake(uint32_t i)
{
    Advance(i);
    m_Awake[i] = 1;
    m_Active.push_back(i);
}

void fly::CBrain::Step()
{
    // 1) Прибывают спайки, отправленные t_dly назад (тот же слот кольца)
    const int p = (int)(m_uStep % (uint64_t)m_nDelay);
    std::vector<SSpike>& q = m_Ring[p];
    const uint32_t* pCptr = m_C.pCptr;
    const uint32_t* pSyn  = m_C.pSyn;
    float*          pG    = m_G.data();
    float*          pIn   = m_In.data();
    const uint8_t*  pAw   = m_Awake.data();
    for (const SSpike& sp : q)
    {
        const uint32_t* s = pSyn + pCptr[sp.uPre];
        const uint32_t* e = pSyn + pCptr[sp.uPre + 1];
        for (; s < e; ++s)
        {
            const uint32_t post = *s & 0x3FFFFu;
            const float    w    = m_aWeight[((int32_t)*s >> 18) + (1 << 13)] * sp.fVal;
            if (pAw[post]) { pG[post] += w; continue; }
            if (pIn[post] == 0.f) m_Touched.push_back(post);   // (повтор в списке не страшен)
            pIn[post] += w;
            if (pIn[post] == 0.f) pIn[post] = 1e-30f;          // ток взаимно погасился — всё равно учесть
        }
    }
    q.clear();

    // Будильник: спящим нейронам со входом органов чувств пора получить свой спайк
    std::vector<uint32_t>& timer = m_Timer[m_uStep & ((1u << 12) - 1)];
    m_Fired.swap(timer);
    for (uint32_t i : m_Fired)
    {
        if (m_Awake[i]) continue;
        if (m_NextIn[i] == m_uStep) Wake(i);
        else if (m_NextIn[i] != UINT64_MAX && m_NextIn[i] > m_uStep && ((m_NextIn[i] - m_uStep) & ((1u << 12) - 1)) == 0)
            timer.push_back(i);   // вход не раньше чем через 4096 шагов — ещё круг
    }
    m_Fired.clear();

    // Спящим, кому пришёл ток: досчитать до этого шага, добавить ток и решить, считать ли шагами
    for (uint32_t i : m_Touched)
    {
        if (pIn[i] == 0.f || m_Awake[i]) continue;
        Advance(i);
        pG[i] += pIn[i];
        pIn[i] = 0.f;
        if (MaySpike(m_V[i], pG[i], m_A[i]))
        {
            m_Awake[i] = 1;
            m_Active.push_back(i);
        }
    }
    m_Touched.clear();

    // 2) Нейроны шаг за шагом: порядок операций — как в lif_steps из brain.py
    const float v0 = m_P.v_0, vrst = m_P.v_rst, vth = m_P.v_th, aInc = m_P.a_inc;
    size_t w = 0;
    for (size_t r = 0; r < m_Active.size(); r++)
    {
        const uint32_t i = m_Active[r];
        float   v = m_V[i], g = pG[i], a = m_A[i], x = m_X[i];
        int32_t rfc = m_Rfc[i];
        const bool driven = m_Rate[i] > 0.f;
        if (driven && m_NextIn[i] == m_uStep)                       // вход бьёт прямо в мембрану
        {
            v = v + m_cKick;
            NextInput(i, m_uStep + 1);
        }
        if (rfc <= 0)
        {
            v = v + (((v0 - v) + g) - a) * m_cMbr;
            g = g * m_cDecG;
        }
        if (rfc > -1073741824) rfc -= 1;
        if (m_bUseA) a = a * m_cDecA;
        if (m_bUseU) x = x + (1.0f - x) * m_cD;
        if (v > vth)
        {
            v = vrst; g = 0.f;
            if (!driven) rfc = m_nRfc;           // у нейронов с внешним входом нет рефрактерности
            m_Cnt[i]++;
            if (!m_Sil[i]) q.push_back({ i, m_bUseU ? (driven ? 1.0f : x) : 1.0f });
            if (m_bUseU) x = x * m_cKeepX;
            if (m_bUseA) a = a + aInc;
            if (m_bLogSpikes) m_SpikeLog.push_back(i);
        }
        m_V[i] = v; pG[i] = g; m_A[i] = a; m_X[i] = x; m_Rfc[i] = rfc;

        // Уснуть: рефрактерность кончилась, а без новых спайков порога не достичь.
        // Нейрон со входом проснётся к своему следующему спайку входа (будильник).
        if (!m_bDense && rfc <= 0 && !MaySpike(v, g, a))
        {
            m_Awake[i] = 0;
            m_Slept[i] = m_uStep + 1;
            if (driven) m_Timer[m_NextIn[i] & ((1u << 12) - 1)].push_back(i);
        }
        else
            m_Active[w++] = i;
    }
    m_Active.resize(w);
    m_uStep++;
}

void fly::CBrain::Run(int nSteps)
{
    for (int k = 0; k < nSteps; k++) Step();
}
