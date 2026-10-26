#include "sim.hpp"
#include "brain.hpp"
#include "brain_blob.hpp"

#include <condition_variable>
#include <math.h>
#include <memory>
#include <mutex>
#include <thread>
#include <time.h>
#include <android/log.h>

namespace
{
    const char* const kInputGroups[fly::kInputCount]   = { "obj_L", "obj_R", "loom_L", "loom_R", "puff", "wind", "sugar", "bitter", "hear_A", "hear_B", "odor_L", "odor_R", "bdn2" };
    const char* const kOutputGroups[fly::kOutputCount] = { "steer_L", "steer_R", "head_L", "head_R", "gf", "fear", "mn9", "groom", "walk", "back", "hear2" };

    double NowS()
    {
        timespec ts;
        clock_gettime(CLOCK_MONOTONIC, &ts);
        return (double)ts.tv_sec + (double)ts.tv_nsec * 1e-9;
    }

    struct SShared
    {
        std::mutex              lock;
        std::condition_variable wake;
        bool                    bStarted = false, bRun = false, bReset = false, bInputs = false;
        float                   fTimeScale = 1.f;
        float                   aIn[fly::kInputCount] = {};
        fly::SSimInfo           info;
        // выходы: накопленные спайки групп и время мозга на момент последнего блока
        uint64_t                aOutSpikes[fly::kOutputCount] = {};
        double                  dOutMs = 0.0;
        uint64_t                aReadSpikes[fly::kOutputCount] = {};
        double                  dReadMs = 0.0;
        std::vector<uint32_t>   spikes;          // для картинки мозга
    };
    SShared             S;
    fly::SConnectome    g_Conn;                  // пишется только потоком мозга до Ready
    std::vector<std::string> g_Paths;

    // ---- брюшная нервная цепочка: свой поток идёт за временем мозга
    constexpr float kVncDt   = 0.0009f;          // шаг цепочки, с (два на блок мозга 1.8 мс)
    constexpr float kAnMaxHz = 150.f;            // восходящие — в мозг не чаще
    struct SVncShared
    {
        std::mutex              lock;
        std::condition_variable wake;
        bool                    bReady = false, bRun = false, bReset = false, bAnNew = false;
        double                  dBrainMs = 0.0;  // докуда дошёл мозг
        std::vector<float>      dn, an;          // DN от мозга, AN в мозг
        fly::SVncOut            out;
        std::vector<float>      rates;           // снимок частот для картинки
    };
    SVncShared          V;
    fly::SVncData       g_Vnc;                   // пишется только потоком мозга до запуска цепочки

    std::string VncPath(const std::string& sBrain)
    {
        const size_t i = sBrain.rfind("fly_brain.bin");
        return i == std::string::npos ? std::string() : sBrain.substr(0, i) + "fly_vnc.bin";
    }

    // Загрузка: вшитая в библиотеку, иначе файл рядом с fly_brain.bin. Нет — мозг без цепочки
    bool LoadVnc()
    {
        std::string sUsed, sErr = "fly_vnc.bin not found";
        bool bOk = false;
        {
            std::lock_guard<std::mutex> l(S.lock);
            S.info.eVnc = fly::ELoad::Loading;
        }
#ifdef FLY_VNC_EMBEDDED
        {
            std::string e;
            if (g_Vnc.LoadMemory(g_FlyVncBin, (size_t)(g_FlyVncBinEnd - g_FlyVncBin), e)) { bOk = true; sUsed = "built into the library"; }
            else sErr = "built-in fly_vnc.bin: " + e;
        }
#endif
        for (size_t i = 0; i < g_Paths.size() && !bOk; i++)
        {
            const std::string p = VncPath(g_Paths[i]);
            if (p.empty()) continue;
            std::string e;
            if (g_Vnc.Load(p.c_str(), e)) { bOk = true; sUsed = p; break; }
            if (e != "can't open file") { sErr = p + ": " + e; break; }
        }
        __android_log_print(bOk ? ANDROID_LOG_INFO : ANDROID_LOG_WARN, "shkbd", "fly: nerve cord %s (%s)",
                            bOk ? "loaded" : "not loaded", bOk ? sUsed.c_str() : sErr.c_str());
        std::lock_guard<std::mutex> l(S.lock);
        S.info.eVnc = bOk ? fly::ELoad::Loading : fly::ELoad::Failed;   // Ready — после калибровки
        S.info.sVncPath = bOk ? sUsed : std::string();
        S.info.sVncError = bOk ? std::string() : sErr;
        S.info.nVncNeurons = g_Vnc.Nv;
        S.info.nVncConnections = g_Vnc.M;
        S.info.nVncDn = g_Vnc.Nd;
        S.info.nVncAn = (uint32_t)g_Vnc.an.size();
        return bOk;
    }

    void VncThread()
    {
        fly::CVnc      vnc(g_Vnc);
        fly::CVncMeter meter(g_Vnc);
        // Опора: размах ритма ног при ходьбе прямо — BDN2 (DNg100) с обеих сторон 15 Гц, 1 с
        {
            if (const std::vector<uint32_t>* g = g_Vnc.Group("dn_DNg100"))
                for (uint32_t n : *g) vnc.SetDnRate(n - g_Vnc.Nv, 15.f);
            for (int k = 0; k < 556; k++) { vnc.Step(kVncDt); vnc.Step(kVncDt); meter.Sample(vnc, 2.f * kVncDt); }
            float aBase[fly::kLegCount], hz = 0.f;
            int   n = 0;
            for (int l = 0; l < fly::kLegCount; l++)
            {
                aBase[l] = meter.Out().aLegAmp[l];
                if (meter.Out().aLegHz[l] > 0.f) { hz += meter.Out().aLegHz[l]; n++; }
            }
            vnc.Reset();
            meter.Reset();
            meter.SetBase(aBase, n ? hz / (float)n : 10.f);
        }
        {
            std::lock_guard<std::mutex> l(V.lock);
            V.out = meter.Out();
            V.bReady = true;
        }
        {
            std::lock_guard<std::mutex> l(S.lock);
            S.info.eVnc = fly::ELoad::Ready;
        }

        std::vector<float> dn(g_Vnc.Nd, 0.f), an;
        double tV = -1.0, dSpeedWall = 0.0, dSpeedSim = 0.0, tSpeed = NowS(), tRates = 0.0;
        for (;;)
        {
            double tB;
            bool   bReset;
            {
                std::unique_lock<std::mutex> l(V.lock);
                if (!V.bRun) V.wake.wait(l, [] { return V.bRun; });
                tB = V.dBrainMs;
                dn = V.dn;
                bReset = V.bReset;
                V.bReset = false;
            }
            if (bReset) { vnc.Reset(); meter.Reset(); }
            if (tV < 0.0 || bReset || tB - tV > 100.0) tV = tB;   // отстали больше чем на 100 мс — не догоняем
            if (tV + 1.8 > tB)
            {
                timespec ts = { 0, 2000000 };
                nanosleep(&ts, nullptr);
                continue;
            }
            for (uint32_t d = 0; d < g_Vnc.Nd; d++) vnc.SetDnRate(d, dn[d]);
            // пачка до ~20 мс, потом свежие частоты DN
            const double t0 = NowS();
            for (int n = 0; tV + 1.8 <= tB && n < 11; n++)
            {
                vnc.Step(kVncDt);
                vnc.Step(kVncDt);
                meter.Sample(vnc, 2.f * kVncDt);
                tV += 1.8;
                dSpeedSim += 1.8;
            }
            dSpeedWall += NowS() - t0;
            fly::AnRates(g_Vnc, vnc, an);
            {
                std::lock_guard<std::mutex> l(V.lock);
                V.an.swap(an);
                V.bAnNew = true;
                V.out = meter.Out();
                if (tV - tRates > 66.0) { V.rates = vnc.Rates(); tRates = tV; }
            }
            if (NowS() - tSpeed > 1.0)
            {
                std::lock_guard<std::mutex> l(S.lock);
                S.info.fVncSpeed = dSpeedWall > 0.0 ? (float)(dSpeedSim / 1000.0 / dSpeedWall) : 0.f;
                dSpeedWall = dSpeedSim = 0.0;
                tSpeed = NowS();
            }
        }
    }

    void Thread()
    {
        // ---- загрузка: вшитый в библиотеку мозг, иначе файл на шлеме
        std::string sUsed, sErr = "fly_brain.bin not found";
        bool bOk = false;
#ifdef FLY_BRAIN_EMBEDDED
        {
            std::string e;
            {
                std::lock_guard<std::mutex> l(S.lock);
                S.info.sPath = "built into the library";
            }
            if (g_Conn.LoadMemory(g_FlyBrainBin, (size_t)(g_FlyBrainBinEnd - g_FlyBrainBin), e, &S.info.fProgress)) { bOk = true; sUsed = "built into the library"; }
            else sErr = "built-in fly_brain.bin: " + e;
        }
#endif
        for (size_t i = 0; i < g_Paths.size() && !bOk; i++)
        {
            const std::string& p = g_Paths[i];
            std::string e;
            {
                std::lock_guard<std::mutex> l(S.lock);
                S.info.sPath = p;
            }
            if (g_Conn.Load(p.c_str(), e, &S.info.fProgress)) { bOk = true; sUsed = p; break; }
            if (e != "can't open file") { sErr = p + ": " + e; break; }   // файл есть, но битый — дальше не ищем
        }
        {
            std::lock_guard<std::mutex> l(S.lock);
            S.info.eLoad = bOk ? fly::ELoad::Ready : fly::ELoad::Failed;
            S.info.sPath = bOk ? sUsed : std::string();
            S.info.sError = bOk ? std::string() : sErr;
            S.info.nNeurons = g_Conn.N;
            S.info.nConnections = g_Conn.M;
        }
        __android_log_print(bOk ? ANDROID_LOG_INFO : ANDROID_LOG_WARN, "shkbd", "fly: brain %s (%s)",
                            bOk ? "loaded" : "not loaded", bOk ? sUsed.c_str() : sErr.c_str());
        if (!bOk) return;

        // ---- группы
        const std::vector<uint32_t>* aIn[fly::kInputCount];
        const std::vector<uint32_t>* aOut[fly::kOutputCount];
        static const std::vector<uint32_t> kEmpty;
        for (int i = 0; i < fly::kInputCount; i++)  { aIn[i]  = g_Conn.Group(kInputGroups[i]);  if (!aIn[i])  aIn[i]  = &kEmpty; }
        for (int i = 0; i < fly::kOutputCount; i++) { aOut[i] = g_Conn.Group(kOutputGroups[i]); if (!aOut[i]) aOut[i] = &kEmpty; }

        // ---- цепочка (необязательно)
        const bool bVnc = LoadVnc();
        std::vector<float> anSet;
        std::unique_ptr<fly::CDnTap> pTap;
        std::vector<uint32_t> walkDn;   // BDN2 (DNg100) в цепочке
        if (bVnc)
        {
            pTap.reset(new fly::CDnTap(g_Vnc, g_Conn.N));
            if (const std::vector<uint32_t>* g = g_Vnc.Group("dn_DNg100"))
                for (uint32_t n : *g) walkDn.push_back(n - g_Vnc.Nv);
            anSet.assign(g_Vnc.an.size(), 0.f);
            {
                std::lock_guard<std::mutex> l(V.lock);   // V.bRun ставит SimRun вместе с S.bRun
                V.dn.assign(g_Vnc.Nd, 0.f);
            }
            std::thread(VncThread).detach();
        }

        fly::CBrain brain(g_Conn, fly::SBrainParams(), (uint64_t)(NowS() * 1e6));
        brain.m_bLogSpikes = true;
        const int    D       = brain.DelaySteps();
        const double dBlockMs = D * brain.Dt();
        float        aCur[fly::kInputCount] = {};
        uint64_t     aSpk[fly::kOutputCount] = {};
        std::vector<uint32_t> spk;
        std::vector<float>    anIn;

        double tWall0 = NowS(), dSim0 = 0.0, dSpeedWall = 0.0, dSpeedSim = 0.0, tSpeed = tWall0;
        float  fLastScale = -1.f;
        std::vector<double> lastSpike(g_Conn.N, -1e9);   // для счётчика "активно за 100 мс"
        uint32_t nActive = 0;
        for (;;)
        {
            float aWant[fly::kInputCount];
            float fScale;
            bool  bReset;
            {
                std::unique_lock<std::mutex> l(S.lock);
                if (!S.bRun)
                {
                    S.wake.wait(l, [] { return S.bRun; });
                    tWall0 = NowS(); dSim0 = brain.TimeMs();   // после паузы — без "догонялок"
                }
                for (int i = 0; i < fly::kInputCount; i++) aWant[i] = S.aIn[i];
                fScale = S.fTimeScale;
                bReset = S.bReset;
                S.bReset = false;
            }
            if (fScale != fLastScale)              // новый масштаб времени — отсчёт темпа заново
            {
                fLastScale = fScale;
                tWall0 = NowS(); dSim0 = brain.TimeMs();
            }
            if (bReset)
            {
                brain.Reset();
                for (float& f : aCur) f = 0.f;
                for (float& f : anSet) f = 0.f;   // Reset выключил и входы восходящих
                if (pTap)
                {
                    pTap->Reset();
                    std::lock_guard<std::mutex> l(V.lock);
                    V.bReset = true;
                }
            }
            for (int i = 0; i < fly::kInputCount; i++)
                if (aWant[i] != aCur[i])
                {
                    aCur[i] = aWant[i];
                    for (uint32_t n : *aIn[i]) brain.SetRate(n, aCur[i]);
                }

            // Блок 1.8 мс (одна синаптическая задержка), потом — сверка с часами
            const double t0 = NowS();
            brain.Run(D);
            const double tCalc = NowS() - t0;
            for (int i = 0; i < fly::kOutputCount; i++)
            {
                uint64_t sum = 0;
                for (uint32_t n : *aOut[i]) sum += brain.Count(n);
                aSpk[i] = sum;
            }
            brain.TakeSpikes(spk);
            const double tNow = brain.TimeMs();
            for (uint32_t n : spk) lastSpike[n] = tNow;

            // ---- мозг ↔ цепочка: DN туда, AN (что пришло с прошлого блока) — сюда
            if (pTap)
            {
                pTap->Add(spk, (float)dBlockMs);
                bool bAn = false;
                {
                    std::lock_guard<std::mutex> l(V.lock);
                    V.dn = pTap->Rates();
                    // Команда "иди" доходит до цепочки ровной частотой: спайки BDN2 в мозге — пуассоновские,
                    // а ритм ног держится только на ровном входе (проверено)
                    if (aCur[fly::kWalkDrive] > 0.f)
                        for (uint32_t d : walkDn) V.dn[d] = aCur[fly::kWalkDrive];
                    V.dBrainMs = tNow;
                    if (V.bAnNew) { anIn.swap(V.an); V.bAnNew = false; bAn = true; }
                }
                if (bAn)
                    for (size_t a = 0; a < anIn.size() && a < anSet.size(); a++)
                    {
                        const float x = anIn[a] < kAnMaxHz ? anIn[a] : kAnMaxHz;
                        if (fabsf(x - anSet[a]) > 1.f || (x == 0.f && anSet[a] != 0.f))
                        {
                            anSet[a] = x;
                            brain.SetRate(g_Vnc.an[a].fafb, x);
                        }
                    }
            }
            dSpeedWall += tCalc; dSpeedSim += dBlockMs;
            {
                std::lock_guard<std::mutex> l(S.lock);
                for (int i = 0; i < fly::kOutputCount; i++) S.aOutSpikes[i] = aSpk[i];
                S.dOutMs = tNow;
                if (S.spikes.size() < 400000) S.spikes.insert(S.spikes.end(), spk.begin(), spk.end());
                S.info.dSimMs = tNow;
                S.info.nAwake = (uint32_t)brain.AwakeCount();
                if (NowS() - tSpeed > 1.0)
                {
                    S.info.fSpeed = dSpeedWall > 0.0 ? (float)(dSpeedSim / 1000.0 / dSpeedWall) : 0.f;
                    dSpeedWall = dSpeedSim = 0.0;
                    tSpeed = NowS();
                    uint32_t n = 0;
                    for (double t : lastSpike) n += (tNow - t <= 100.0);
                    nActive = n;
                }
                S.info.nActive = nActive;
            }

            // Темп: мозг идёт за часами × масштаб. Отстал больше чем на 100 мс — не догоняем
            const double dDue = (NowS() - tWall0) * 1000.0 * fScale + dSim0;
            const double dAhead = tNow - dDue;
            if (dAhead > 0.0)
            {
                // спим с запасом 3 мс: мозг считает пачками, а не по 1.8 мс с пробуждением на каждый блок
                const double s = dAhead / 1000.0 / (fScale > 1e-3f ? fScale : 1e-3f) + 0.003;
                timespec ts = { (time_t)s, (long)((s - (double)(time_t)s) * 1e9) };
                nanosleep(&ts, nullptr);
            }
            else if (dAhead < -100.0)
            {
                tWall0 = NowS();
                dSim0  = tNow;
            }
        }
    }
}

const char* fly::InputGroup(int i)  { return i >= 0 && i < kInputCount ? kInputGroups[i] : ""; }
const char* fly::OutputGroup(int i) { return i >= 0 && i < kOutputCount ? kOutputGroups[i] : ""; }

void fly::SimLoad(const std::vector<std::string>& paths)
{
    std::lock_guard<std::mutex> l(S.lock);
    if (S.bStarted) return;
    S.bStarted   = true;
    S.info.eLoad = ELoad::Loading;
    g_Paths      = paths;
    std::thread(Thread).detach();
}

fly::SSimInfo fly::SimInfo()
{
    std::lock_guard<std::mutex> l(S.lock);
    return S.info;
}

void fly::SimRun(bool bRun)
{
    {
        std::lock_guard<std::mutex> l(S.lock);
        S.bRun = bRun;
    }
    {
        std::lock_guard<std::mutex> l(V.lock);
        V.bRun = bRun;
    }
    S.wake.notify_all();
    V.wake.notify_all();
}

void fly::SimSetTimeScale(float fScale)
{
    std::lock_guard<std::mutex> l(S.lock);
    S.fTimeScale = fScale < 0.02f ? 0.02f : (fScale > 1.f ? 1.f : fScale);
}

void fly::SimSetInputs(const float aHz[kInputCount])
{
    std::lock_guard<std::mutex> l(S.lock);
    for (int i = 0; i < kInputCount; i++) S.aIn[i] = aHz[i] > 0.f ? aHz[i] : 0.f;
}

void fly::SimReset()
{
    std::lock_guard<std::mutex> l(S.lock);
    S.bReset = true;
}

bool fly::SimReadout(float aHz[kOutputCount], double& dMs)
{
    std::lock_guard<std::mutex> l(S.lock);
    dMs = S.dOutMs - S.dReadMs;
    if (S.info.eLoad != ELoad::Ready || dMs <= 0.0) return false;
    for (int i = 0; i < kOutputCount; i++)
    {
        const std::vector<uint32_t>* g = g_Conn.Group(kOutputGroups[i]);
        const double n = g && !g->empty() ? (double)g->size() : 1.0;
        // счётчики после Reset начинаются заново — тогда берём их как есть
        const uint64_t d = S.aOutSpikes[i] >= S.aReadSpikes[i] ? S.aOutSpikes[i] - S.aReadSpikes[i] : S.aOutSpikes[i];
        aHz[i] = (float)((double)d / n * 1000.0 / dMs);
        S.aReadSpikes[i] = S.aOutSpikes[i];
    }
    S.dReadMs = S.dOutMs;
    return true;
}

void fly::SimTakeSpikes(std::vector<uint32_t>& out)
{
    std::lock_guard<std::mutex> l(S.lock);
    out.swap(S.spikes);
    S.spikes.clear();
}

const fly::SConnectome* fly::SimConnectome()
{
    std::lock_guard<std::mutex> l(S.lock);
    return S.info.eLoad == ELoad::Ready ? &g_Conn : nullptr;
}

bool fly::SimVnc(SVncOut& out)
{
    std::lock_guard<std::mutex> l(V.lock);
    if (!V.bReady) return false;
    out = V.out;
    return true;
}

void fly::SimVncRates(std::vector<float>& out)
{
    std::lock_guard<std::mutex> l(V.lock);
    out = V.rates;
}

const fly::SVncData* fly::SimVncData()
{
    std::lock_guard<std::mutex> l(V.lock);
    return V.bReady ? &g_Vnc : nullptr;
}
