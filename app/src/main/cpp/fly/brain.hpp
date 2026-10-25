#pragma once

#include "connectome.hpp"

#include <stdint.h>
#include <vector>

// Мозг мухи: LIF-модель всего коннектома, уравнения и порядок операций — как в
// fly-connectome/brain.py (Shiu et al. 2024), только на процессоре:
//
//   dv/dt = (v_0 − v + g − a) / t_mbr     спайк при v > v_th, затем v = v_rst и рефрактерность t_rfc
//   dg/dt = −g / tau                      спайк соседа j добавляет w_syn · (знак · число синапсов j→i) · x_j
//   вход органов чувств — пуассоновские спайки, каждый добавляет w_syn · f_poi прямо в мембрану
//
// "Противоэпилептика" fly-connectome: утомление синапсов (x, U, tau_d) и адаптация (a, a_inc, tau_a).
//
// Считаем событийно. Спайк доставляется только по исходящим связям. Нейрон, который до порога
// не дойдёт, "спит": между входами его шаги Эйлера — линейная рекуррентность, и состояние через
// k шагов считается формулой, когда к нему придёт следующий спайк. По той же формуле видно,
// поднимется ли мембрана до порога (максимум суммы двух экспонент), — такой нейрон считается
// шаг за шагом, как в brain.py. Нейроны со входом органов чувств и в рефрактерности — тоже.
namespace fly
{
    struct SBrainParams
    {
        float v_0 = -52.f, v_rst = -52.f, v_th = -45.f;   // мВ
        float t_mbr = 20.f, tau = 5.f, t_rfc = 2.2f, t_dly = 1.8f;   // мс
        float w_syn = 0.275f, f_poi = 250.f;
        float a_inc = 0.1f, tau_a = 3000.f;               // адаптация (a_inc = 0 — выкл)
        float U = 0.1f, tau_d = 300.f;                    // утомление синапсов (U = 0 — выкл)
        float dt = 0.1f;                                  // мс
    };
    SBrainParams OriginalModel();   // без противоэпилептики — исходная модель Shiu et al.

    class CBrain
    {
    public:
        // bDense — эталон: каждый нейрон каждый шаг, без "сна" (для проверки)
        CBrain(const SConnectome& c, const SBrainParams& p, uint64_t uSeed, bool bDense = false);

        void Reset();                              // всё в покой, входы выключены
        void SetRate(uint32_t i, float fHz);       // частота пуассоновского входа, 0 — выкл
        void Silence(uint32_t i, bool bSilent);    // спайки нейрона никуда не доходят
        void Run(int nSteps);

        int      DelaySteps() const { return m_nDelay; }
        float    Dt() const         { return m_P.dt; }
        uint64_t Steps() const      { return m_uStep; }
        double   TimeMs() const     { return (double)m_uStep * m_P.dt; }
        uint32_t Count(uint32_t i) const { return m_Cnt[i]; }   // спайков с Reset
        size_t   AwakeCount() const { return m_Active.size(); }

        // Кто спайкнул с прошлого вызова (для картинки мозга); копится, только если m_bLogSpikes
        bool m_bLogSpikes = false;
        void TakeSpikes(std::vector<uint32_t>& out) { out.swap(m_SpikeLog); m_SpikeLog.clear(); }

    private:
        struct SSpike { uint32_t uPre; float fVal; };

        void   Step();
        void   Wake(uint32_t i);                          // в счёт шаг за шагом
        void   Advance(uint32_t i);                       // спящий: состояние на начало текущего шага
        bool   MaySpike(float v, float g, float a) const; // дойдёт ли до порога без новых входов
        void   NextInput(uint32_t i, uint64_t uFrom);     // следующий пуассоновский спайк входа
        double Pow(const std::vector<float>& t, double dLog, uint64_t k) const;
        double Uniform(uint32_t i, uint64_t uSalt) const;

        const SConnectome& m_C;
        SBrainParams       m_P;
        uint64_t           m_uSeed;
        bool               m_bDense;
        int                m_nDelay, m_nRfc;
        bool               m_bUseA, m_bUseU;
        float              m_cMbr, m_cDecG, m_cDecA, m_cD, m_cKeepX, m_cKick, m_cPScale;
        double             m_dLogDecA, m_dLog1mD, m_dLogBeta, m_dLogGamma;
        double             m_dBeta, m_dGamma, m_dAlpha;  // множители за шаг: мембрана, ток, адаптация
        double             m_dPsp;                       // пик ПСП на единицу тока: max_k c·(β^k − γ^k)/(β − γ)
        float              m_aWeight[1 << 14];           // знак × число синапсов -> мВ (как float32 в brain.py)
        std::vector<float> m_PowBeta, m_PowGamma, m_PowAlpha, m_PowRec;   // степени на k шагов

        std::vector<float>    m_V, m_G, m_A, m_X, m_Rate;
        std::vector<int32_t>  m_Rfc;
        std::vector<uint32_t> m_Cnt;
        std::vector<uint64_t> m_Slept;                   // спящий: его состояние — на начало этого шага
        std::vector<uint64_t> m_NextIn;                  // шаг следующего спайка входа (вход органов чувств)
        std::vector<uint32_t> m_Timer[1 << 12];          // кого разбудить на шаге (по модулю 4096)
        std::vector<uint32_t> m_Fired;
        std::vector<uint8_t>  m_Awake, m_Sil;
        std::vector<float>    m_In;                      // входящий ток спящих за этот шаг
        std::vector<uint32_t> m_Touched;                 // спящие, которым пришёл ток на этом шаге
        std::vector<uint32_t> m_Active;                  // кого считаем шаг за шагом
        std::vector<SSpike>   m_Ring[64];                // спайки "в пути" по слотам задержки
        std::vector<uint32_t> m_SpikeLog;
        uint64_t              m_uStep = 0;
    };
}
