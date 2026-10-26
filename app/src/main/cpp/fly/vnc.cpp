#include "vnc.hpp"

#include <math.h>
#include <stdio.h>
#include <string.h>
#include <zlib.h>

static_assert(sizeof(fly::SVncData::SHeader) == 28, "header layout");

namespace
{
    // Параметры нейронов — средние значения распределений из статьи (configs/neuron_params/default.yaml)
    constexpr float kTau = 0.02f, kGain = 1.f, kTheta = 7.5f, kCap = 200.f, kW = 0.03f;
    constexpr float kEps = 1e-4f;   // Гц: ниже — нейрон молчит, его выход не считаем

    bool HeaderOk(const fly::SVncData::SHeader& h)
    {
        return memcmp(h.magic, "FLYV", 4) == 0 && h.uVersion == 1 && h.Nv > 0 && h.Nv < (1u << 18);
    }
}

const std::vector<uint32_t>* fly::SVncData::Group(const char* pszName) const
{
    for (const SConnectome::SGroup& g : groups)
        if (g.sName == pszName) return &g.idx;
    return nullptr;
}

bool fly::SVncData::Load(const char* pszPath, std::string& sError, volatile float* pProgress)
{
    FILE* f = fopen(pszPath, "rb");
    if (!f) { sError = "can't open file"; return false; }
    std::vector<unsigned char> file;
    fseek(f, 0, SEEK_END);
    const long n = ftell(f);
    fseek(f, 0, SEEK_SET);
    file.resize(n > 0 ? (size_t)n : 0);
    const bool bRead = n > 0 && fread(file.data(), 1, file.size(), f) == file.size();   // файл маленький (~2 МБ)
    fclose(f);
    SHeader h {};
    if (!bRead || file.size() < sizeof(h)) { sError = "file is damaged or incomplete"; return false; }
    memcpy(&h, file.data(), sizeof(h));
    if (!HeaderOk(h)) { sError = "not a fly_vnc.bin (version 1)"; return false; }
    // буфер файла сейчас исчезнет — данные всегда копируются в свои
    const bool bOk = Unpack(file.data() + sizeof(h), file.size() - sizeof(h), h, sError);
    if (pProgress) *pProgress = 1.f;
    return bOk;
}

bool fly::SVncData::Unpack(const unsigned char* pBody, size_t nBody, const SHeader& h, std::string& sError)
{
    m_Data.assign((h.uSize + 3) / 4, 0u);
    if (!(h.uFlags & 1))
    {
        if (nBody < h.uSize) { sError = "data is truncated"; return false; }
        memcpy(m_Data.data(), pBody, h.uSize);
    }
    else
    {
        uLongf nOut = h.uSize;
        if (uncompress(reinterpret_cast<Bytef*>(m_Data.data()), &nOut, pBody, (uLong)nBody) != Z_OK || nOut != h.uSize)
        {
            sError = "data is damaged or incomplete";
            m_Data.clear();
            return false;
        }
    }
    return Parse(reinterpret_cast<const unsigned char*>(m_Data.data()), h, sError);
}

bool fly::SVncData::LoadMemory(const void* pData, size_t nBytes, std::string& sError, volatile float* pProgress)
{
    const unsigned char* p = static_cast<const unsigned char*>(pData);
    SHeader h {};
    if (nBytes < sizeof(h)) { sError = "not a fly_vnc.bin (version 1)"; return false; }
    memcpy(&h, p, sizeof(h));
    if (!HeaderOk(h)) { sError = "not a fly_vnc.bin (version 1)"; return false; }
    const unsigned char* pBody = p + sizeof(h);
    const size_t         nBody = nBytes - sizeof(h);
    bool bOk;
    if (!(h.uFlags & 1) && ((uintptr_t)pBody & 3u) == 0 && nBody >= h.uSize)
    {
        m_Data.clear();                  // без сжатия и выровнено — читаем прямо отсюда
        bOk = Parse(pBody, h, sError);
    }
    else bOk = Unpack(pBody, nBody, h, sError);
    if (pProgress) *pProgress = 1.f;
    return bOk;
}

bool fly::SVncData::Parse(const unsigned char* p, const SHeader& h, std::string& sError)
{
    const size_t nNodes = (size_t)h.Nv + h.Nd;
    size_t off = 0;
    const auto Need = [&](size_t n) { return off + n <= h.uSize; };
    if (!Need((nNodes + 1) * 4 + (size_t)h.M * 4 + nNodes * 4 + (size_t)h.Nv * 7)) { sError = "file is truncated"; return false; }
    pCptr  = reinterpret_cast<const uint32_t*>(p + off); off += (nNodes + 1) * 4;
    pSyn   = reinterpret_cast<const uint32_t*>(p + off); off += (size_t)h.M * 4;
    pSize  = reinterpret_cast<const float*>(p + off);    off += nNodes * 4;
    pPos   = reinterpret_cast<const int16_t*>(p + off);  off += (size_t)h.Nv * 6;
    pClass = p + off;                                     off += h.Nv;
    if (pCptr[nNodes] != h.M) { sError = "connection table doesn't match its header"; return false; }

    const auto U8  = [&](uint8_t& v)  { if (!Need(1)) return false; v = p[off++]; return true; };
    const auto U32 = [&](uint32_t& v) { if (!Need(4)) return false; memcpy(&v, p + off, 4); off += 4; return true; };
    dnFafb.assign(h.Nd, {});
    for (uint32_t d = 0; d < h.Nd; d++)
    {
        uint8_t n = 0;
        if (!U8(n)) { sError = "DN table is truncated"; return false; }
        dnFafb[d].resize(n);
        for (uint8_t k = 0; k < n; k++) if (!U32(dnFafb[d][k])) { sError = "DN table is truncated"; return false; }
    }
    uint32_t nAn = 0;
    if (!U32(nAn)) { sError = "AN table is truncated"; return false; }
    an.resize(nAn);
    for (SAn& a : an)
    {
        uint8_t n = 0;
        if (!U32(a.fafb) || !U8(n)) { sError = "AN table is truncated"; return false; }
        a.vnc.resize(n);
        for (uint8_t k = 0; k < n; k++)
            if (!U32(a.vnc[k]) || a.vnc[k] >= h.Nv) { sError = "AN table is broken"; return false; }
    }
    uint32_t nGroups = 0;
    if (!U32(nGroups)) { sError = "group table is truncated"; return false; }
    groups.clear();
    for (uint32_t k = 0; k < nGroups; k++)
    {
        uint8_t nName = 0;
        if (!U8(nName) || !Need(nName)) { sError = "group table is truncated"; return false; }
        SConnectome::SGroup g;
        g.sName.assign(reinterpret_cast<const char*>(p + off), nName); off += nName;
        uint32_t n = 0;
        if (!U32(n) || !Need((size_t)n * 4)) { sError = "group table is truncated"; return false; }
        g.idx.resize(n);
        memcpy(g.idx.data(), p + off, (size_t)n * 4); off += (size_t)n * 4;
        for (uint32_t i : g.idx) if (i >= nNodes) { sError = "group refers to a missing neuron"; return false; }
        groups.push_back(std::move(g));
    }
    Nv = h.Nv; Nd = h.Nd; M = h.M;
    return true;
}

// ---------------------------------------------------------------- модель
fly::CVnc::CVnc(const SVncData& d) : m_D(d)
{
    m_W.resize(d.M);
    for (uint32_t s = 0; s < d.M; s++) m_W[s] = kW * (float)((int32_t)d.pSyn[s] >> 18);
    m_Tau.assign(d.Nv, kTau);
    m_Gain.resize(d.Nv);
    m_Th.resize(d.Nv);
    for (uint32_t i = 0; i < d.Nv; i++)
    {
        const float sz = d.pSize[i] > 1e-3f ? d.pSize[i] : 1.f;
        m_Gain[i] = kGain / sz / kCap;   // a / r_max
        m_Th[i]   = kTheta * sz;
    }
    m_R.assign(d.Nv, 0.f);
    m_X.assign(d.Nv, 0.f);
    m_Dn.assign(d.Nd, 0.f);
    m_Stamp.assign(d.Nv, 0u);
}

void fly::CVnc::Reset()
{
    for (float& r : m_R) r = 0.f;
    for (float& x : m_X) x = 0.f;
    for (float& r : m_Dn) r = 0.f;
    m_Active.clear();
}

void fly::CVnc::SetDnRate(uint32_t d, float fHz)
{
    if (d < m_D.Nd) m_Dn[d] = fHz > 0.f ? fHz : 0.f;
}

void fly::CVnc::Step(float fDt)
{
    // Вход Σ w r — только от тех, кто сейчас активен (почти вся цепочка молчит)
    if (++m_uStamp == 0) { for (uint32_t& s : m_Stamp) s = 0; m_uStamp = 1; }
    m_Touched.clear();
    const auto Push = [&](uint32_t node, float r)
    {
        for (uint32_t s = m_D.pCptr[node], e = m_D.pCptr[node + 1]; s < e; s++)
        {
            const uint32_t post = m_D.pSyn[s] & 0x3FFFFu;
            if (m_Stamp[post] != m_uStamp) { m_Stamp[post] = m_uStamp; m_X[post] = 0.f; m_Touched.push_back(post); }
            m_X[post] += m_W[s] * r;
        }
    };
    for (uint32_t j : m_Active) Push(j, m_R[j]);
    for (uint32_t d = 0; d < m_D.Nd; d++) if (m_Dn[d] > kEps) Push(m_D.Nv + d, m_Dn[d]);

    // Активные без входа гаснут, получившие вход — по уравнению
    for (uint32_t j : m_Active)
        if (m_Stamp[j] != m_uStamp) { m_Stamp[j] = m_uStamp; m_X[j] = 0.f; m_Touched.push_back(j); }
    m_Next.clear();
    for (uint32_t i : m_Touched)
    {
        const float x   = m_X[i] - m_Th[i];
        const float act = x > 0.f ? kCap * tanhf(m_Gain[i] * x) : 0.f;
        float r = m_R[i] + fDt / m_Tau[i] * (act - m_R[i]);
        if (r < kEps) r = 0.f; else m_Next.push_back(i);
        m_R[i] = r;
    }
    m_Active.swap(m_Next);
}

// ---------------------------------------------------------------- мозг ↔ цепочка
fly::CDnTap::CDnTap(const SVncData& d, uint32_t nBrain, float fTauMs) : m_fTauMs(fTauMs)
{
    m_Ptr.assign(nBrain + 1, 0u);
    for (const std::vector<uint32_t>& l : d.dnFafb)
        for (uint32_t n : l) if (n < nBrain) m_Ptr[n + 1]++;
    for (uint32_t n = 0; n < nBrain; n++) m_Ptr[n + 1] += m_Ptr[n];
    m_Dn.resize(m_Ptr[nBrain]);
    std::vector<uint32_t> fill(m_Ptr.begin(), m_Ptr.end() - 1);
    m_Inv.assign(d.Nd, 0.f);
    for (uint32_t k = 0; k < d.Nd; k++)
    {
        uint32_t n = 0;
        for (uint32_t b : d.dnFafb[k]) if (b < nBrain) { m_Dn[fill[b]++] = k; n++; }
        m_Inv[k] = n ? 1.f / (float)n : 0.f;
    }
    m_Cnt.assign(d.Nd, 0.f);
    m_Rate1.assign(d.Nd, 0.f);
    m_Rate.assign(d.Nd, 0.f);
}

void fly::CDnTap::Reset()
{
    for (float& r : m_Rate1) r = 0.f;
    for (float& r : m_Rate) r = 0.f;
    for (float& c : m_Cnt) c = 0.f;
}

void fly::CDnTap::Add(const std::vector<uint32_t>& spikes, float fMs)
{
    if (fMs <= 0.f) return;
    const uint32_t nBrain = (uint32_t)m_Ptr.size() - 1;
    for (uint32_t n : spikes)
        if (n < nBrain)
            for (uint32_t k = m_Ptr[n]; k < m_Ptr[n + 1]; k++) m_Cnt[m_Dn[k]] += 1.f;
    const float a = fMs < m_fTauMs ? fMs / m_fTauMs : 1.f, toHz = 1000.f / fMs;
    for (size_t d = 0; d < m_Rate.size(); d++)
    {
        m_Rate1[d] += (m_Cnt[d] * m_Inv[d] * toHz - m_Rate1[d]) * a;
        m_Rate[d]  += (m_Rate1[d] - m_Rate[d]) * a;
        if (m_Rate[d] < 1e-3f && m_Rate1[d] < 1e-3f) m_Rate[d] = m_Rate1[d] = 0.f;
        m_Cnt[d] = 0.f;
    }
}

void fly::AnRates(const SVncData& d, const CVnc& v, std::vector<float>& out)
{
    out.resize(d.an.size());
    for (size_t a = 0; a < d.an.size(); a++)
    {
        float s = 0.f;
        for (uint32_t i : d.an[a].vnc) s += v.Rate(i);
        out[a] = d.an[a].vnc.empty() ? 0.f : s / (float)d.an[a].vnc.size();
    }
}

// ---------------------------------------------------------------- что делает цепочка
fly::CVncMeter::CVncMeter(const SVncData& d)
{
    static const std::vector<uint32_t> kEmpty;
    const auto G = [&](const char* psz) { const std::vector<uint32_t>* p = d.Group(psz); return p ? p : &kEmpty; };
    static const char* const kLegs[kLegCount] = { "L1_all", "L2_all", "L3_all", "R1_all", "R2_all", "R3_all" };
    for (int l = 0; l < kLegCount; l++) m_aLeg[l] = G(kLegs[l]);
    static const char* const kPairs[5][2] = { { "wing_power_L", "wing_power_R" }, { "wing_steer_L", "wing_steer_R" },
                                              { "haltere_L", "haltere_R" }, { "ttm_L", "ttm_R" }, { "neck_L", "neck_R" } };
    for (int k = 0; k < 5; k++) for (int s = 0; s < 2; s++) m_aPair[k][s] = G(kPairs[k][s]);
    Reset();
}

void fly::CVncMeter::Reset()
{
    m_iHist = m_nHist = 0;
    m_fSinceRhythm = 0.f;
    const SVncOut prev = m_Out;
    m_Out = SVncOut();
    m_Out.bReady  = prev.bReady;
    m_Out.fBaseHz = prev.fBaseHz;
    for (int l = 0; l < kLegCount; l++) m_Out.aLegBase[l] = prev.aLegBase[l];
}

void fly::CVncMeter::SetBase(const float aBase[kLegCount], float fBaseHz)
{
    for (int l = 0; l < kLegCount; l++) m_Out.aLegBase[l] = aBase[l];
    m_Out.fBaseHz = fBaseHz;
    m_Out.bReady  = true;
}

void fly::CVncMeter::Sample(const CVnc& v, float fDt)
{
    for (int l = 0; l < kLegCount; l++)
    {
        float s = 0.f;
        for (uint32_t i : *m_aLeg[l]) s += v.Rate(i);
        m_aHist[l][m_iHist] = s;
    }
    m_iHist = (m_iHist + 1) % kHist;
    if (m_nHist < kHist) m_nHist++;
    float* aOut[5][2] = { { &m_Out.aWingPower[0], &m_Out.aWingPower[1] }, { &m_Out.aWingSteer[0], &m_Out.aWingSteer[1] },
                          { &m_Out.aHaltere[0], &m_Out.aHaltere[1] }, { &m_Out.aTtm[0], &m_Out.aTtm[1] }, { &m_Out.aNeck[0], &m_Out.aNeck[1] } };
    for (int k = 0; k < 5; k++)
        for (int s = 0; s < 2; s++)
        {
            float r = 0.f;
            for (uint32_t i : *m_aPair[k][s]) r += v.Rate(i);
            *aOut[k][s] = m_aPair[k][s]->empty() ? 0.f : r / (float)m_aPair[k][s]->size();
        }
    m_Out.nActive = v.ActiveCount();

    // Ритм ног — раз в 0.1 с: без медленного дрейфа (скользящее среднее ±75 мс), первый пик
    // автокорреляции выше 0.4 на задержке 27..270 мс (шаг дрозофилы — 5..20 Гц)
    if ((m_fSinceRhythm += fDt) < 0.1f) return;
    m_fSinceRhythm = 0.f;
    const int W = m_nHist < (int)(0.5f / fDt) ? m_nHist : (int)(0.5f / fDt);
    const int lagLo = (int)(0.027f / fDt), lagHi = (int)(0.27f / fDt) < W / 2 ? (int)(0.27f / fDt) : W / 2;
    const int H = (int)(0.075f / fDt);
    float  x[kHist];
    double sum[kHist + 1];
    for (int l = 0; l < kLegCount; l++)
    {
        sum[0] = 0.0;
        for (int i = 0; i < W; i++) { x[i] = m_aHist[l][(m_iHist - W + i + kHist) % kHist]; sum[i + 1] = sum[i] + x[i]; }
        const size_t n = m_aLeg[l]->empty() ? 1 : m_aLeg[l]->size();
        m_Out.aLegRate[l] = W > 0 ? (float)(sum[W] / W / (double)n) : 0.f;
        m_Out.aLegHz[l] = 0.f;
        m_Out.aLegAmp[l] = 0.f;
        double v0 = 0.0;
        for (int i = 0; i < W; i++)
        {
            const int a = i - H > 0 ? i - H : 0, b = i + H + 1 < W ? i + H + 1 : W;
            x[i] -= (float)((sum[b] - sum[a]) / (b - a));
            v0 += (double)x[i] * x[i];
        }
        // ритм — когда размах заметен (σ суммы > 0.1 Гц) и есть настоящий пик, а не спад
        if (lagHi <= lagLo + 2 || v0 < 0.01 * W) continue;
        float prev2 = 0.f, prev1 = 0.f;
        for (int lag = lagLo - 1; lag <= lagHi + 1; lag++)
        {
            double c = 0.0;
            for (int i = 0; i + lag < W; i++) c += (double)x[i] * x[i + lag];
            const float cur = (float)(c / v0);
            if (lag >= lagLo + 1 && prev1 > prev2 && prev1 >= cur && prev1 > 0.4f)
            {
                // размах — периодическая часть разброса: σ × √(пик автокорреляции)
                m_Out.aLegHz[l]  = 1.f / ((float)(lag - 1) * fDt);
                m_Out.aLegAmp[l] = (float)sqrt(v0 / W * prev1);
                break;
            }
            prev2 = prev1; prev1 = cur;
        }
    }
}
