#pragma once

#include "connectome.hpp"

#include <stddef.h>
#include <stdint.h>
#include <string>
#include <vector>

// Брюшная нервная цепочка (грудной ганглий) из коннектома BANC — fly_vnc.bin (tools/fly/make_vnc.py).
// Модель скоростей Pugliese et al. 2025: τ dr/dt = −r + [r_max tanh(a / r_max · (Σ w r − θ))]+,
// w = 0.03 × число синапсов (ACh +, прочие −), a и θ — по размеру нейрона. Входы — нисходящие
// нейроны (DN) с частотами из мозга FAFB; выходы — мотонейроны ног, крыльев, жужжалец и
// восходящие нейроны (AN), которые возвращают сигнал в мозг.
namespace fly
{
    struct SVncData
    {
        SVncData() = default;
        SVncData(const SVncData&) = delete;
        SVncData& operator=(const SVncData&) = delete;

        uint32_t        Nv = 0, Nd = 0, M = 0;   // нейронов цепочки, DN-входов, связей
        const uint32_t* pCptr  = nullptr;        // исходящие связи узла p (0..Nv-1 — цепочка, Nv.. — DN)
        const uint32_t* pSyn   = nullptr;        // биты 0..17 — нейрон цепочки, 18..31 — знак × число синапсов
        const float*    pSize  = nullptr;        // Nv + Nd: размер нейрона / медиана
        const int16_t*  pPos   = nullptr;        // Nv × 3, 0.25 мкм (координаты BANC)
        const uint8_t*  pClass = nullptr;        // 1 интернейрон, 2 мотонейрон, 3 восходящий, 4 сенсорный

        std::vector<std::vector<uint32_t>> dnFafb;           // DN → нейроны мозга FAFB (их средняя частота)
        struct SAn { uint32_t fafb; std::vector<uint32_t> vnc; };
        std::vector<SAn> an;                                  // нейрон FAFB ← нейроны цепочки (средняя частота)
        std::vector<SConnectome::SGroup> groups;
        const std::vector<uint32_t>* Group(const char* pszName) const;

        bool Load(const char* pszPath, std::string& sError, volatile float* pProgress = nullptr);
        bool LoadMemory(const void* pData, size_t nBytes, std::string& sError, volatile float* pProgress = nullptr);

        struct SHeader { char magic[4]; uint32_t uVersion, Nv, Nd, M, uFlags, uSize; };

    private:
        bool Parse(const unsigned char* p, const SHeader& h, std::string& sError);
        bool Unpack(const unsigned char* pBody, size_t nBody, const SHeader& h, std::string& sError);
        std::vector<uint32_t> m_Data;
    };

    class CVnc
    {
    public:
        explicit CVnc(const SVncData& d);

        void  Reset();
        void  SetDnRate(uint32_t d, float fHz);       // частота DN d (0..Nd-1), Гц
        void  Step(float fDt);                        // шаг, с (устойчиво до ~2 мс)
        float Rate(uint32_t i) const { return m_R[i]; }
        const std::vector<float>& Rates() const { return m_R; }
        uint32_t ActiveCount() const { return (uint32_t)m_Active.size(); }

    private:
        const SVncData&       m_D;
        std::vector<float>    m_W;                    // вес связи: 0.03 × знак × синапсы
        std::vector<float>    m_Tau, m_Gain, m_Th;    // τ, a / r_max, θ (a и θ — по размеру)
        std::vector<float>    m_R, m_X, m_Dn;
        std::vector<uint32_t> m_Active, m_Touched, m_Next;
        std::vector<uint32_t> m_Stamp;
        uint32_t              m_uStamp = 1;
    };

    // Мозг → цепочка: частота DN — средняя по его нейронам FAFB (тот же тип и сторона), сглаженная
    // дважды по fTauMs. Ритм ног в модели цепочки держится на ровном тоническом входе: спайки
    // мозга как есть (или сглаженные один раз) ритм сбивают — проверено
    class CDnTap
    {
    public:
        CDnTap(const SVncData& d, uint32_t nBrain, float fTauMs = 50.f);
        void Reset();
        void Add(const std::vector<uint32_t>& spikes, float fMs);   // спайки мозга за fMs
        const std::vector<float>& Rates() const { return m_Rate; }

    private:
        std::vector<uint32_t> m_Ptr, m_Dn;   // нейрон FAFB → его DN
        std::vector<float>    m_Inv, m_Cnt, m_Rate1, m_Rate;
        float                 m_fTauMs;
    };

    // Цепочка → мозг: частоты восходящих нейронов для их двойников в FAFB (по d.an)
    void AnRates(const SVncData& d, const CVnc& v, std::vector<float>& out);

    // Что делает цепочка. Ноги — L1 L2 L3 R1 R2 R3 (передняя, средняя, задняя)
    enum ELeg { kL1, kL2, kL3, kR1, kR2, kR3, kLegCount };
    struct SVncOut
    {
        bool     bReady = false;
        float    aLegHz[kLegCount]   = {};   // ритм ноги (автокорреляция за 0.5 с без медленного дрейфа), 0 — ритма нет
        float    aLegRate[kLegCount] = {};   // мотонейроны ноги, Гц на нейрон (среднее за 0.5 с)
        float    aLegAmp[kLegCount]  = {};   // размах ритма (сумма мотонейронов ноги, Гц), 0 — ритма нет
        float    aLegBase[kLegCount] = {};   // размах ритма при ходьбе прямо (BDN2 15 Гц) — опора
        float    fBaseHz = 0.f;              // ритм ног при ходьбе прямо
        float    aWingPower[2] = {}, aWingSteer[2] = {}, aHaltere[2] = {}, aTtm[2] = {}, aNeck[2] = {};   // L, R; Гц на нейрон сейчас
        uint32_t nActive = 0;
    };

    class CVncMeter
    {
    public:
        explicit CVncMeter(const SVncData& d);
        void Reset();
        void Sample(const CVnc& v, float fDt);   // раз в ~1.8 мс времени цепочки
        void SetBase(const float aBase[kLegCount], float fBaseHz);   // опора (ходьба прямо) — дальше Out().bReady
        const SVncOut& Out() const { return m_Out; }

    private:
        static constexpr int kHist = 512;
        const std::vector<uint32_t>* m_aLeg[kLegCount];
        const std::vector<uint32_t>* m_aPair[5][2];
        float    m_aHist[kLegCount][kHist];
        int      m_iHist = 0, m_nHist = 0;
        float    m_fSinceRhythm = 0.f;
        SVncOut  m_Out;
    };
}
