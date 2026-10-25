#include "fly.hpp"
#include "unity_bridge.hpp"
#include "../config.hpp"
#include "../fly/body.hpp"
#include "../fly/draw.hpp"
#include "../openxr_hook/openxr_hook.hpp"

#include <math.h>
#include <mutex>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <vector>
#include <android/log.h>

namespace
{
    using flymod::EState;
    using flymod::EStim;
    using flymod::ETool;

    // ---------------------------------------------------------------- векторы (оси Unity)
    Vector3 V(float x, float y, float z)            { return Vector3(x, y, z); }
    Vector3 Add(const Vector3& a, const Vector3& b) { return V(a.x + b.x, a.y + b.y, a.z + b.z); }
    Vector3 Sub(const Vector3& a, const Vector3& b) { return V(a.x - b.x, a.y - b.y, a.z - b.z); }
    Vector3 Mul(const Vector3& a, float s)          { return V(a.x * s, a.y * s, a.z * s); }
    float   Dot(const Vector3& a, const Vector3& b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
    float   Len(const Vector3& a)                   { return sqrtf(Dot(a, a)); }
    Vector3 Flat(const Vector3& a)                  { return V(a.x, 0.f, a.z); }
    float   Clamp(float v, float lo, float hi)      { return v < lo ? lo : (v > hi ? hi : v); }
    float   Wrap(float a)                           { while (a > 3.14159265f) a -= 6.2831853f; while (a < -3.14159265f) a += 6.2831853f; return a; }

    uint64_t NowNs()
    {
        timespec ts;
        clock_gettime(CLOCK_MONOTONIC, &ts);
        return (uint64_t)ts.tv_sec * 1000000000ull + (uint64_t)ts.tv_nsec;
    }

    uint32_t g_uRng = 0x9E3779B9u;
    float Rand01() { g_uRng ^= g_uRng << 13; g_uRng ^= g_uRng >> 17; g_uRng ^= g_uRng << 5; return (float)(g_uRng >> 8) / 16777216.f; }

    // ---------------------------------------------------------------- числа из fly-connectome
    // Пороги и сглаживание — tiktok/senses.py (THRESH, Filt); органы чувств, еда, взлёт,
    // скорость поворота — show.py. Расстояния в мм мухи, время — время мухи (с).
    constexpr float kTauFilt = 0.06f, kTauMN9 = 0.2f, kTauHear = 0.15f;
    constexpr float kTakeoff = 45.f, kStartle = 25.f;
    constexpr float kEatOn = 8.f, kEatKeep = 3.f, kGroomOn = 35.f, kGroomKeep = 12.f;
    constexpr float kTurnGain = 0.12f, kHeadGain = 0.008f;
    constexpr float kFlightS = 0.75f, kGfCool = 1.5f, kStartleCool = 0.4f;
    constexpr float kEatR = 1.7f, kEatTime = 1.4f;
    constexpr float kPuffDur = 1.5f, kPuffTau = 0.8f, kSugarDur = 3.f, kBitterDur = 1.5f, kSoundDur = 0.6f, kLoomDur = 0.4f;
    constexpr float kPulsePer = 0.25f, kPulseOn = 0.10f;   // касания сахара 4 Гц по 100 мс
    constexpr float kLoomMin = 0.2f;                       // ниже — не надвигание (как LOOM_MIN в senses.py)
    // Тело и мир (правила тела, а не мозга)
    constexpr float kWalkMm   = 7.f;     // мм/с при команде ходьбы 1 (CPG NeuroMechFly)
    constexpr float kTrackMm  = 1.6f;    // "колея" для дифференциального поворота
    constexpr float kHopAway  = 10.f, kHopSide = 3.f, kHopArc = 5.f;
    constexpr float kDropMm   = 0.9f;    // радиус капли сахара
    constexpr int   kMaxDrops = 8;
    constexpr float kLeashMm  = 45.f;    // бродя, не уходит от места появления дальше
    constexpr float kHandR    = 0.045f;  // м: рука с контроллером для глаза мухи
    constexpr float kHeadR    = 0.09f;   // м: голова игрока
    constexpr float kSwatMps  = 0.6f;    // м/с: быстрее — прихлопнул, медленнее — коснулся

    enum ECmd { kCmdRemove, kCmdClearSugar, kCmdResetBrain, kCmdStim0 };

    // ---------------------------------------------------------------- общее с меню и хуком (g_Lock)
    std::mutex        g_Lock;
    flymod::SOptions  g_Opt;
    ETool             g_eTool = ETool::Off;
    std::vector<int>  g_Cmds;
    flymod::SStatus   g_Status;
    fly::SScene       g_Scene;
    uint64_t          g_uSceneNs = 0;
    std::string       g_sBeam;

    void Message(const std::string& s)
    {
        std::lock_guard<std::mutex> lock(g_Lock);
        g_Status.sMessage = s;
        g_Status.iMessageId++;
    }

    // ---------------------------------------------------------------- только главный поток
    struct SDrop { Vector3 p; float fLeft = 1.f; bool bSeen = false; };

    struct SFly
    {
        bool      bExists = false, bSwatted = false;
        Vector3   pos = V(0.f, 0.f, 0.f), home = V(0.f, 0.f, 0.f), normal = V(0.f, 1.f, 0.f);
        float     fYaw = 0.f;                 // + — к +X мира
        fly::CBody body;
        // поведение
        EState    eState = EState::Walk;
        float     fFilt[fly::kOutputCount] = {};
        float     fHunger = 0.7f, fSticky = 0.f, fMn9Last = -9.f, fGfCool = 0.f, fStartleT = -9.f;
        bool      bProb = false;
        bool      bFlying = false;
        float     fFlightT0 = 0.f, fYaw0 = 0.f, fDpsi = 0.f;
        Vector3   flyFrom = V(0.f, 0.f, 0.f), flyTo = V(0.f, 0.f, 0.f);
        float     fSwatT = 0.f, fSwatYaw = 0.f;
        float     fAvoidT = 0.f, fAvoidSign = 1.f, fRestT = 0.f, fNextRest = 5.f;
        float     fTurn = 0.f, fHead = 0.f;
        int       nEaten = 0, nTakeoffs = 0;
        // стимулы из меню: когда начались (время мухи), < 0 — нет
        float     aStim[6] = { -1.f, -1.f, -1.f, -1.f, -1.f, -1.f };
        float     fTouchT = -9.f;             // мягкое касание рукой → щетинки
        // угрозы: левая рука, правая рука, голова игрока
        float     aTheta[3] = {}, aDTheta[3] = {};
        bool      aThetaOk[3] = {};
        float     aIn[fly::kInputCount] = {};
    };
    SFly                g_Fly;
    float               g_fT = 0.f;           // время мухи
    std::vector<SDrop>  g_Drops;
    bool                g_bNeedReset = false; // следующей мухе — свежий мозг
    bool                g_bGunOn = false, g_bTriggerDown = false;
    bool                g_bLure = false;
    Vector3             g_Lure = V(0.f, 0.f, 0.f), g_LurePrev = V(0.f, 0.f, 0.f);
    float               g_fLureMotion = 0.f;
    Vector3             g_aHandPrev[2] = { V(0.f, 0.f, 0.f), V(0.f, 0.f, 0.f) };
    float               g_aHandSpeed[2] = {};
    bool                g_aHandOk[2] = {};
    Vector3             g_Cam = V(0.f, 0.f, 0.f);
    float               g_fScale = 1.f;

    // ---------------------------------------------------------------- геометрия мухи
    Vector3 Forward(float yaw) { return V(sinf(yaw), 0.f, cosf(yaw)); }
    Vector3 LeftOf(float yaw)  { return V(-cosf(yaw), 0.f, sinf(yaw)); }

    // Точка мухи (мм, оси мухи без наклона) → мир
    Vector3 FlyPoint(const SFly& f, float s, float x, float y, float z)
    {
        return Add(f.pos, Add(Mul(LeftOf(f.fYaw), -x * s), Add(V(0.f, y * s, 0.f), Mul(Forward(f.fYaw), z * s))));
    }

    // show.py side_split: объект в направлении to → сигнал левому / правому глазу. + угла — влево
    float SideSplit(const SFly& f, const Vector3& to, float fStrength, float fBack, float& l, float& r)
    {
        const float fx = Dot(Forward(f.fYaw), to), lx = Dot(LeftOf(f.fYaw), to);
        const float ang = atan2f(lx, fx);
        l = r = 0.f;
        if (fabsf(ang) > fBack) return ang;
        l = fStrength * Clamp(sinf(ang) * 1.2f + 0.25f, 0.f, 1.f);
        r = fStrength * Clamp(-sinf(ang) * 1.2f + 0.25f, 0.f, 1.f);
        return ang;
    }

    // Пол под точкой: луч сверху вниз; true — нашёлся горизонтальный (нормаль вверх)
    bool Ground(const Vector3& at, float s, ubridge::SHit& hit, float fAbove = 3.f, float fBelow = 3.f)
    {
        if (!ubridge::Raycast(Add(at, V(0.f, fAbove * s, 0.f)), V(0.f, -1.f, 0.f), (fAbove + fBelow) * s, hit)) return false;
        return hit.normal.y > 0.6f;
    }

    // ---------------------------------------------------------------- стимулы
    float StimAge(const SFly& f, EStim e) { const float t0 = f.aStim[(int)e]; return t0 < 0.f ? -1.f : g_fT - t0; }
    bool  Pulse(float t)                  { return fmodf(t, kPulsePer) < kPulseOn; }

    // ---------------------------------------------------------------- луч правой руки
    void GunOff()
    {
        g_bLure = false;
        if (!g_bGunOn) return;
        g_bGunOn       = false;
        g_bTriggerDown = false;
        if (g_pOpenXRHook)
        {
            g_pOpenXRHook->SetSpawnBeam(false, -1.f);
            g_pOpenXRHook->m_Input.m_bMaskRightTrigger.store(false, std::memory_order_relaxed);
        }
    }

    void Spawn(const Vector3& at, const Vector3& normal)
    {
        const bool bMove = g_Fly.bExists && !g_Fly.bSwatted;
        if (!bMove)
        {
            const bool bReset = g_bNeedReset || g_Fly.bSwatted;
            g_Fly = SFly();
            g_Fly.fHunger = 0.55f + 0.3f * Rand01();   // как в show.py
            if (bReset) fly::SimReset();
            g_bNeedReset = false;
        }
        g_Fly.bExists = true;
        g_Fly.bFlying = false;
        g_Fly.pos     = at;
        g_Fly.home    = at;
        g_Fly.normal  = normal;
        const Vector3 toCam = Flat(Sub(g_Cam, at));
        g_Fly.fYaw    = Len(toCam) > 1e-4f ? atan2f(toCam.x, toCam.z) : 0.f;   // лицом к игроку
        fly::SimRun(true);
        Message(bMove ? "The fly is here now" : "A fly! Its brain is running: 138,639 neurons");
    }

    void TickTool(ETool eTool, float s)
    {
        COpenXRHook* pHook = g_pOpenXRHook;
        if (eTool == ETool::Off || !pHook) { GunOff(); return; }

        XrVector3f dirHead, posHead;
        Vector3    hand, dir;
        if (!pHook->GetRightAimInHead(dirHead, &posHead) ||
            !ubridge::HeadToWorld(V(posHead.x, posHead.y, -posHead.z), V(dirHead.x, dirHead.y, -dirHead.z), hand, dir))
        {
            pHook->SetSpawnBeam(false, -1.f);
            g_bLure = false;
            return;
        }
        const Vector3 origin = Add(hand, Mul(dir, 0.05f * g_fScale));
        ubridge::SHit hit {};
        const bool  bHit   = ubridge::Raycast(origin, dir, cfg::kSpawnBeamMaxM * g_fScale, hit);
        const bool  bFloor = bHit && hit.normal.y > 0.5f;
        pHook->SetSpawnBeam(true, bHit ? hit.fDistance / g_fScale : -1.f);
        g_bGunOn = true;

        const bool  bOnMenu  = pHook->IsPointerOnMenu();
        pHook->m_Input.m_bMaskRightTrigger.store(!bOnMenu, std::memory_order_relaxed);
        const float fTrigger = pHook->m_Input.Right.TriggerValue();
        const bool  bDown    = g_bTriggerDown ? fTrigger > cfg::kTriggerRelease : fTrigger >= cfg::kTriggerPress;
        const bool  bPress   = bDown && !g_bTriggerDown && !bOnMenu;
        g_bTriggerDown = bDown;

        std::string sLabel;
        switch (eTool)
        {
        case ETool::Spawn:
            sLabel = !bFloor ? "Point at a floor or a table" : (g_Fly.bExists && !g_Fly.bSwatted ? "Move the fly here" : "Spawn a fly here");
            if (bPress)
            {
                if (fly::SimInfo().eLoad != fly::ELoad::Ready) Message("The brain isn't loaded yet: see the Fly page");
                else if (!bFloor)                               Message("A fly needs a floor or a table to stand on");
                else                                            Spawn(hit.point, hit.normal);
            }
            break;
        case ETool::Sugar:
        {
            char sz[64];
            snprintf(sz, sizeof(sz), bFloor ? "Sugar drop (%d of %d)" : "Point at a floor or a table", (int)g_Drops.size() + 1 > kMaxDrops ? kMaxDrops : (int)g_Drops.size() + 1, kMaxDrops);
            sLabel = sz;
            if (bPress && bFloor)
            {
                if ((int)g_Drops.size() >= kMaxDrops)
                {
                    g_Drops.erase(g_Drops.begin());
                    Message("8 drops at most: the oldest one is gone");
                }
                SDrop d;
                d.p = hit.point;
                g_Drops.push_back(d);
            }
            else if (bPress) Message("Sugar goes on a floor or a table");
            break;
        }
        case ETool::Lure:
            g_bLure = bDown && !bOnMenu && bFloor;
            sLabel  = g_bLure ? "Lure: move the dot slowly" : "Lure: hold the trigger";
            if (g_bLure) g_Lure = hit.point;
            break;
        default: break;
        }
        std::lock_guard<std::mutex> lock(g_Lock);
        g_sBeam = sLabel;
    }

    // ---------------------------------------------------------------- руки игрока
    void TickHands(float fDt)
    {
        COpenXRHook* pHook = g_pOpenXRHook;
        for (int i = 0; i < 2; i++)
        {
            XrVector3f dirHead, posHead;
            Vector3    p, d;
            const bool bOk = pHook && pHook->GetHandInHead(i == 1, false, dirHead, &posHead) &&
                             ubridge::HeadToWorld(V(posHead.x, posHead.y, -posHead.z), V(dirHead.x, dirHead.y, -dirHead.z), p, d);
            if (bOk && g_aHandOk[i] && fDt > 1e-4f)
            {
                const float v = Len(Sub(p, g_aHandPrev[i])) / fDt / g_fScale;
                g_aHandSpeed[i] += (v - g_aHandSpeed[i]) * Clamp(fDt / 0.05f, 0.f, 1.f);
            }
            g_aHandOk[i] = bOk;
            if (bOk) g_aHandPrev[i] = p;
        }
    }

    // ---------------------------------------------------------------- полёт (show.py: 0.75 с, дуга)
    void StartFlight(SFly& f, Vector3 away, float s)
    {
        away = Flat(away);
        const float l = Len(away);
        away = l > 1e-5f ? Mul(away, 1.f / l) : Forward(f.fYaw);
        const Vector3 side = Mul(V(-away.z, 0.f, away.x), Rand01() < 0.5f ? -1.f : 1.f);
        Vector3 to = Add(f.pos, Add(Mul(away, kHopAway * s), Mul(side, kHopSide * s)));

        // стена по пути — садимся перед ней; пола в точке посадки нет — прыжок на месте
        const Vector3 up = V(0.f, 3.f * s, 0.f), path = Sub(to, f.pos);
        const float   fPath = Len(path);
        ubridge::SHit hit {};
        if (fPath > 1e-5f && ubridge::Raycast(Add(f.pos, up), Mul(path, 1.f / fPath), fPath, hit))
            to = Add(f.pos, Mul(path, fmaxf(0.f, hit.fDistance - 2.f * s) / fPath));
        if (Ground(to, s, hit, kHopArc + 4.f, kHopArc + 8.f)) to.y = hit.point.y;
        else                                                  to = f.pos;

        f.bFlying   = true;
        f.fFlightT0 = g_fT;
        f.flyFrom   = f.pos;
        f.flyTo     = to;
        f.fYaw0     = f.fYaw;
        f.fDpsi     = Wrap(atan2f(away.x, away.z) - f.fYaw);
        f.nTakeoffs++;
        Message("Takeoff! The giant fiber DNp01 fired");
    }

    void Swat(SFly& f)
    {
        f.bSwatted = true;
        f.fSwatT   = g_fT;
        f.fSwatYaw = f.fYaw;
        f.eState   = EState::Swatted;
        fly::SimRun(false);              // мёртвый мозг гаснет
        Message("Swatted: it didn't take off in time. Spawn a new fly with the beam");
    }

    // ---------------------------------------------------------------- органы чувств
    void Senses(SFly& f, float s, float dt, bool& bTouching, SDrop*& pNear, Vector3& threat, float& fThreat)
    {
        float* in = f.aIn;
        for (int i = 0; i < fly::kInputCount; i++) in[i] = 0.f;
        const Vector3 head = FlyPoint(f, s, 0.f, 0.66f, 1.02f);

        // ---- еда и приманка → LC10 (show.py: сила 6 / (d + 2), d в мм)
        float fL = 0.f, fR = 0.f;
        for (SDrop& d : g_Drops)
        {
            const Vector3 to = Flat(Sub(d.p, f.pos));
            const float   dist = Len(to) / s;
            float l, r;
            SideSplit(f, to, Clamp(6.f / (dist + 2.f), 0.f, 1.f) * d.fLeft, 3.14159265f * 165.f / 180.f, l, r);
            fL += l; fR += r;
            if (l + r > 0.3f && !d.bSeen && f.eState != EState::Eat)
            {
                d.bSeen = true;
                Message("Sugar spotted: LC10 steers the fly through DNa02");
            }
        }
        if (g_bLure)
        {
            const Vector3 to = Flat(Sub(g_Lure, f.pos));
            float l, r;
            SideSplit(f, to, Clamp(6.f / (Len(to) / s + 2.f), 0.f, 1.f) * (0.3f + 0.7f * g_fLureMotion), 3.14159265f * 165.f / 180.f, l, r);
            fL += l; fR += r;
        }
        in[fly::kObjL] = 60.f * fminf(fL, 1.f);
        in[fly::kObjR] = 60.f * fminf(fR, 1.f);

        // ---- надвигание: руки и голова игрока → LPLC2 / LC4. Угловой размер θ = 2 atan(R / d),
        // сигнал — пока θ растёт (show.py: θ / 0.9; тут ещё и по скорости роста, иначе спокойная
        // рука рядом — тоже угроза)
        float tL = 0.f, tR = 0.f;
        fThreat = 0.f;
        COpenXRHook* pHook = g_pOpenXRHook;
        for (int k = 0; k < 3; k++)
        {
            Vector3 src;
            float   R;
            if (k < 2) { if (!g_aHandOk[k]) { f.aThetaOk[k] = false; continue; } src = g_aHandPrev[k]; R = kHandR * g_fScale; }
            else       { src = g_Cam; R = kHeadR * g_fScale; (void)pHook; }
            const Vector3 to   = Sub(src, head);
            const float   dist = fmaxf(Len(to), R * 1.05f);
            const float   th   = 2.f * atanf(R / dist);
            if (f.aThetaOk[k] && dt > 0.f)
            {
                const float dth = (th - f.aTheta[k]) / dt;
                f.aDTheta[k] += (dth - f.aDTheta[k]) * Clamp(dt / 0.03f, 0.f, 1.f);
            }
            if (dt > 0.f || !f.aThetaOk[k]) { f.aTheta[k] = th; f.aThetaOk[k] = true; }
            const float loom = Clamp(th / 0.9f, 0.f, 1.f) * Clamp(f.aDTheta[k] / 1.0f, 0.f, 1.f);
            const float lv   = Clamp((loom - kLoomMin) / (1.f - kLoomMin), 0.f, 1.f);
            if (lv <= 0.f) continue;
            float l, r;
            SideSplit(f, Flat(to), lv, 3.14159265f * 175.f / 180.f, l, r);
            tL = fmaxf(tL, l); tR = fmaxf(tR, r);
            if (lv > fThreat) { fThreat = lv; threat = src; }
        }
        in[fly::kLoomL] = 80.f * tL;
        in[fly::kLoomR] = 80.f * tR;

        // ---- касание: рука у груди мухи. Медленно — щетинки (чистка), быстро — прихлопнул
        const Vector3 thorax = FlyPoint(f, s, 0.f, 0.62f, 0.35f);
        for (int k = 0; k < 2; k++)
        {
            if (!g_aHandOk[k] || f.bFlying || f.bSwatted) continue;
            if (Len(Sub(g_aHandPrev[k], thorax)) > kHandR * g_fScale * 0.6f + 1.4f * s) continue;
            if (g_aHandSpeed[k] > kSwatMps && g_Opt.bSwat) { Swat(f); return; }
            if (g_fT - f.fTouchT > kPuffDur)
            {
                f.fTouchT = g_fT;
                Message("Touched: head bristles fire, grooming neurons listen");
            }
        }

        // ---- еда под головой → сахарные нейроны (show.py: 80 + 120 × голод)
        const float rEat = kEatR + (f.eState == EState::Eat ? 0.7f : 0.f);
        pNear = nullptr;
        float best = 1e9f;
        for (SDrop& d : g_Drops)
        {
            const float dist = Len(Flat(Sub(d.p, head))) / s;
            if (dist < rEat && dist < best) { best = dist; pNear = &d; }
        }
        bTouching = pNear && !f.bFlying;
        if (bTouching) in[fly::kSugar] = 80.f + 120.f * f.fHunger;

        // ---- щетинки: касание, липкий сироп после еды (show.py: 90 × sticky), стимул Puff
        float puff = f.fSticky > 0.05f ? 90.f * f.fSticky : 0.f, wind = 0.f;
        const float aPuffAge[2] = { StimAge(f, EStim::Puff), g_fT - f.fTouchT };
        for (float tp : aPuffAge)
            if (tp >= 0.f && tp < kPuffDur)
            {
                puff = fmaxf(puff, 60.f * expf(-tp / kPuffTau));
                wind = fmaxf(wind, 40.f * expf(-tp / kPuffTau));
            }
        in[fly::kPuff] = puff;
        in[fly::kWind] = wind;

        // ---- стимулы из меню (senses.py: сахар 300 Гц и горькое 100 Гц касаниями 4 Гц, звук — удар)
        const float ts = StimAge(f, EStim::Sugar);
        if (ts >= 0.f && ts < kSugarDur && Pulse(ts)) in[fly::kSugar] = fmaxf(in[fly::kSugar], 300.f);
        const float tb = StimAge(f, EStim::Bitter);
        if (tb >= 0.f && tb < kBitterDur && Pulse(tb)) in[fly::kBitter] = 100.f;
        const float th = StimAge(f, EStim::Sound);
        if (th >= 0.f && th < kSoundDur)
        {
            const float env = expf(-th / 0.1f);
            in[fly::kHearA] = fminf(130.f, 100.f * env);
            in[fly::kHearB] = fminf(130.f, 40.f * env);
        }
        const float tlL = StimAge(f, EStim::LoomL), tlR = StimAge(f, EStim::LoomR);
        if (tlL >= 0.f && tlL < kLoomDur) in[fly::kLoomL] = fmaxf(in[fly::kLoomL], 60.f);
        if (tlR >= 0.f && tlR < kLoomDur) in[fly::kLoomR] = fmaxf(in[fly::kLoomR], 60.f);
    }

    // ---------------------------------------------------------------- шаг по полу
    // Правило тела: впереди обрыв, ступенька или стена — разворот (у живой мухи это делают
    // лапки и брюшная нервная цепочка, которых в модели нет)
    void Walk(SFly& f, float s, float v, float w, float dt)
    {
        f.fYaw = Wrap(f.fYaw - w * dt);
        if (v <= 1e-4f) return;
        const Vector3 fwd  = Forward(f.fYaw);
        const Vector3 next = Add(f.pos, Mul(fwd, v * dt * s));
        ubridge::SHit hit {};
        bool bBlocked = false;
        // пол проверяем под головой (1.3 мм впереди), а стоит муха — на полу под грудью
        if (ubridge::Raycast(Add(f.pos, V(0.f, 0.6f * s, 0.f)), fwd, (1.6f + v * dt) * s, hit)) bBlocked = true;
        else if (!Ground(Add(next, Mul(fwd, 1.3f * s)), s, hit, 1.5f, 1.5f))                     bBlocked = true;
        else if (!Ground(next, s, hit, 1.5f, 1.5f))                                              bBlocked = true;
        if (bBlocked)
        {
            if (f.fAvoidT <= 0.f)
            {
                // в какую сторону свободнее: пол есть слева или справа
                ubridge::SHit h2 {};
                const bool bL = Ground(Add(f.pos, Mul(Forward(f.fYaw - 0.9f), 2.f * s)), s, h2, 1.5f, 1.5f);
                const bool bR = Ground(Add(f.pos, Mul(Forward(f.fYaw + 0.9f), 2.f * s)), s, h2, 1.5f, 1.5f);
                f.fAvoidSign = bL == bR ? (Rand01() < 0.5f ? -1.f : 1.f) : (bL ? 1.f : -1.f);
            }
            f.fAvoidT = 0.6f;
            return;
        }
        f.pos = V(next.x, hit.point.y, next.z);
        f.normal = Add(Mul(f.normal, 0.8f), Mul(hit.normal, 0.2f));
    }

    // ---------------------------------------------------------------- поведение (show.py)
    void Behave(SFly& f, float s, float dt)
    {
        bool    bTouching = false;
        SDrop*  pNear = nullptr;
        Vector3 threat = V(0.f, 0.f, 0.f);
        float   fThreat = 0.f;
        Senses(f, s, dt, bTouching, pNear, threat, fThreat);
        if (f.bSwatted) return;
        fly::SimSetInputs(f.aIn);

        const float* F  = f.fFilt;
        const float  gf = F[fly::kGF], mn9 = F[fly::kMN9], groom = F[fly::kGroom];
        f.fHunger = fminf(1.f, f.fHunger + 0.03f * dt);      // время жизни ускорено, как в show.py
        f.fSticky *= expf(-dt / 0.8f);
        f.fGfCool -= dt;
        if (mn9 > 3.f) f.fMn9Last = g_fT;

        fly::SBodyCmd cmd;
        // ---- взлёт: гигантское волокно DNp01
        if (!f.bFlying && gf > kTakeoff && f.fGfCool <= 0.f)
        {
            Vector3 away;
            if (fThreat > 0.f) away = Sub(f.pos, threat);
            else
            {
                // угроза из меню (надвигание слева / справа) — от неё; иначе вперёд
                const float side = f.aIn[fly::kLoomL] - f.aIn[fly::kLoomR];
                away = fabsf(side) > 1.f ? Mul(LeftOf(f.fYaw), side > 0.f ? -1.f : 1.f) : Forward(f.fYaw);
            }
            StartFlight(f, away, s);
        }
        else if (!f.bFlying && gf > kStartle && gf <= kTakeoff && g_fT - f.fStartleT > kStartleCool)
            f.fStartleT = g_fT;

        const float fStartleAge = g_fT - f.fStartleT;
        cmd.bStartle = fStartleAge < 0.12f;
        const bool bProb = mn9 > kEatOn || (f.bProb && mn9 > kEatKeep);
        f.bProb = bProb;
        cmd.fProboscis = bProb ? Clamp(mn9 / 12.f, 0.f, 1.f) : 0.f;
        f.fHead = tanhf(kHeadGain * (F[fly::kHeadL] - F[fly::kHeadR]));
        cmd.fHead = f.fHead;

        if (f.bFlying)
        {
            const float u = (g_fT - f.fFlightT0) / kFlightS;
            if (u >= 1.f)
            {
                f.bFlying = false;
                f.pos     = f.flyTo;
                f.fYaw    = Wrap(f.fYaw0 + f.fDpsi);
                f.fGfCool = kGfCool;
                f.home    = f.pos;
            }
            else
            {
                const float e = 0.5f - 0.5f * cosf(3.14159265f * u);
                f.pos  = Add(f.flyFrom, Mul(Sub(f.flyTo, f.flyFrom), e));
                f.pos.y += kHopArc * s * sinf(3.14159265f * u);
                f.fYaw = Wrap(f.fYaw0 + f.fDpsi * e);
                cmd.bFlight = true;
            }
            f.eState = EState::Flight;
        }

        const bool bGroom = !f.bFlying && !bTouching && (groom > kGroomOn || (f.eState == EState::Groom && groom > kGroomKeep));
        const bool bEat   = !f.bFlying && bTouching && (mn9 > kEatOn || (f.eState == EState::Eat && g_fT - f.fMn9Last < 0.8f));
        if (f.bFlying) {}
        else if (bGroom) { f.eState = EState::Groom; cmd.bGroom = true; }
        else if (bEat)
        {
            // У капли: подползти, пока хоботок до неё не достаёт (правило тела)
            f.eState = EState::Eat;
            cmd.bEat = true;
            const Vector3 to   = Flat(Sub(pNear->p, FlyPoint(f, s, 0.f, 0.66f, 1.02f)));
            const float   dist = Len(to) / s, reach = kDropMm * (0.35f + 0.65f * pNear->fLeft) * 0.5f + 0.2f;
            if (dist > reach)
            {
                const float ang = atan2f(Dot(LeftOf(f.fYaw), to), Dot(Forward(f.fYaw), to));
                cmd.fFwd = fminf(1.5f, (dist - reach) * 4.f) * Clamp(cosf(ang), 0.f, 1.f);
                cmd.fYaw = Clamp(ang * 3.f, -2.f, 2.f);
                Walk(f, s, cmd.fFwd, cmd.fYaw, dt);
            }
        }
        else if (bProb)  { f.eState = EState::Proboscis; }
        else
        {
            // ---- ходьба: руль DNa01/DNa02 → дифференциальная команда ногам (show.py)
            f.fTurn = tanhf(kTurnGain * (F[fly::kSteerL] - F[fly::kSteerR]));
            float aL = Clamp(1.f - fmaxf(f.fTurn, 0.f), 0.f, 1.2f), aR = Clamp(1.f + fminf(f.fTurn, 0.f), 0.f, 1.2f);
            const bool bSeek   = f.aIn[fly::kObjL] + f.aIn[fly::kObjR] > 3.f;
            const bool bThreat = fmaxf(f.aIn[fly::kLoomL], f.aIn[fly::kLoomR]) > 0.15f * 80.f;
            f.eState = bThreat ? EState::Threat : (bSeek ? EState::Seek : EState::Walk);

            // правила тела: пауза, когда бродит без цели; не уходить далеко от места появления
            float w = 0.f;
            if (!bSeek && !bThreat)
            {
                if (!g_Opt.bWander) { aL = aR = 0.f; f.eState = EState::Rest; }
                else if (f.fRestT > 0.f)
                {
                    f.fRestT -= dt;
                    aL = aR = 0.f;
                    f.eState = EState::Rest;
                }
                else if ((f.fNextRest -= dt) <= 0.f)
                {
                    f.fRestT    = 0.8f + 2.f * Rand01();
                    f.fNextRest = 4.f + 6.f * Rand01();
                }
                const Vector3 toHome = Flat(Sub(f.home, f.pos));
                if (Len(toHome) / s > kLeashMm)
                {
                    const float ang = atan2f(Dot(LeftOf(f.fYaw), toHome), Dot(Forward(f.fYaw), toHome));
                    w += Clamp(ang * 1.5f, -2.f, 2.f);
                }
            }
            float v = kWalkMm * (aL + aR) * 0.5f;
            w += (aR - aL) * kWalkMm / kTrackMm * 0.5f;
            if (f.fAvoidT > 0.f)
            {
                f.fAvoidT -= dt;
                v = kWalkMm * 0.25f;
                w = f.fAvoidSign * 3.f;
                f.eState = EState::Avoid;
            }
            cmd.fFwd = v;
            cmd.fYaw = w;
            Walk(f, s, v, w, dt);
        }

        // ---- еда съедается, пока работает хоботок (show.py)
        if (f.eState == EState::Eat && pNear && mn9 > 3.f)
        {
            pNear->fLeft = fmaxf(0.f, pNear->fLeft - dt / kEatTime);
            f.fHunger    = fmaxf(0.f, f.fHunger - 0.35f * dt / kEatTime);
            if (pNear->fLeft <= 0.f)
            {
                g_Drops.erase(g_Drops.begin() + (pNear - g_Drops.data()));
                f.nEaten++;
                f.fSticky = 1.f;
                f.eState  = EState::Walk;
                Message("Drop finished. Sticky syrup on the head: bristles call for grooming");
            }
        }
        f.body.Update(dt, cmd);
    }

    // ---------------------------------------------------------------- подпись
    void Label(const SFly& f, const char*& pszState, const char*& pszCause, char* szLine2, size_t n)
    {
        const float* F = f.fFilt;
        szLine2[0] = 0;
        const bool bStartle = g_fT - f.fStartleT < 0.6f && !f.bFlying && !f.bSwatted;
        if (bStartle)
        {
            pszState = "Startle"; pszCause = "DNp01 giant fiber";
            snprintf(szLine2, n, "DNp01 giant fiber · %.0f Hz", F[fly::kGF]);
            return;
        }
        switch (f.eState)
        {
        case EState::Swatted:   pszState = "Swatted";         pszCause = "didn't take off in time"; break;
        case EState::Flight:    pszState = "Takeoff!";        pszCause = "DNp01 giant fiber";
                                snprintf(szLine2, n, "DNp01 giant fiber · %.0f Hz", F[fly::kGF]); return;
        case EState::Groom:     pszState = "Grooming";        pszCause = "DNg84 / DNg35";
                                snprintf(szLine2, n, "DNg84 / DNg35 · %.0f Hz", F[fly::kGroom]); return;
        case EState::Eat:       pszState = "Drinking sugar";  pszCause = "MN9 proboscis motor neuron";
                                snprintf(szLine2, n, "MN9 proboscis · %.0f Hz", F[fly::kMN9]); return;
        case EState::Proboscis: pszState = "Proboscis out";   pszCause = "MN9 proboscis motor neuron";
                                snprintf(szLine2, n, "MN9 proboscis · %.0f Hz", F[fly::kMN9]); return;
        case EState::Threat:    pszState = "Threat!";         pszCause = "LPLC2 / LC4 looming";
                                snprintf(szLine2, n, "LPLC2 / LC4 → giant fiber %.0f Hz", F[fly::kGF]); return;
        case EState::Seek:      pszState = f.fTurn > 0.3f ? "Turning left to food" : (f.fTurn < -0.3f ? "Turning right to food" : "Going for food");
                                pszCause = "LC10 → DNa01 / DNa02";
                                snprintf(szLine2, n, "DNa L %.0f · R %.0f Hz", F[fly::kSteerL], F[fly::kSteerR]); return;
        case EState::Rest:      pszState = "Resting";         pszCause = "body rule: a pause"; break;
        case EState::Avoid:     pszState = "Turning back";    pszCause = "body rule: edge or wall"; break;
        default:                pszState = fabsf(f.fTurn) > 0.3f ? (f.fTurn > 0.f ? "Turning left" : "Turning right") : "Walking";
                                pszCause = fabsf(f.fTurn) > 0.3f ? "DNa01 / DNa02" : "walking rhythm of the body";
                                if (fabsf(f.fTurn) > 0.3f) { snprintf(szLine2, n, "DNa L %.0f · R %.0f Hz", F[fly::kSteerL], F[fly::kSteerR]); return; }
                                break;
        }
        snprintf(szLine2, n, "%s", pszCause);
    }

    void Publish(const fly::SRig& rig, const flymod::SOptions& opt, float s)
    {
        const SFly& f = g_Fly;
        const char* pszState = "";
        const char* pszCause = "";
        char        szLine2[96] = {};
        if (f.bExists) Label(f, pszState, pszCause, szLine2, sizeof(szLine2));

        std::lock_guard<std::mutex> lock(g_Lock);
        fly::SScene& sc = g_Scene;
        sc.rig        = rig;
        sc.bFly       = f.bExists;
        sc.fMmToWorld = s;
        if (f.bExists)
        {
            Vector3 o = f.pos;
            sc.fYaw = f.fYaw;
            // наклон по полу: нормаль в осях мухи
            const Vector3 n = f.normal;
            const float   nl = Len(n) > 1e-5f ? 1.f / Len(n) : 1.f;
            const float   nx = -Dot(LeftOf(f.fYaw), n) * nl, ny = n.y * nl, nz = Dot(Forward(f.fYaw), n) * nl;
            sc.fRoll  = asinf(Clamp(-nx, -1.f, 1.f));
            sc.fPitch = atan2f(nz, ny);
            if (f.bFlying) sc.fPitch = sc.fRoll = 0.f;
            if (f.bSwatted)
            {
                // "лапками вверх" (show.py): переворот вокруг продольной оси за 0.45 с
                const float u = Clamp((g_fT - f.fSwatT) / 0.45f, 0.f, 1.f);
                sc.fRoll  = 3.14159265f * (0.5f - 0.5f * cosf(3.14159265f * u));
                sc.fPitch = 0.f;
                o.y += (1.0f * u + 1.2f * sinf(3.14159265f * u)) * s;
            }
            sc.aOrigin[0] = o.x; sc.aOrigin[1] = o.y; sc.aOrigin[2] = o.z;
            f.body.Pose(sc.pose);
        }
        sc.nDrops = 0;
        for (const SDrop& d : g_Drops)
        {
            if (sc.nDrops >= 8) break;
            float* p = sc.aDrops[sc.nDrops++];
            p[0] = d.p.x; p[1] = d.p.y; p[2] = d.p.z;
            p[3] = kDropMm * s * (0.35f + 0.65f * d.fLeft);
        }
        snprintf(sc.szLabel,  sizeof(sc.szLabel),  "%s", f.bExists && opt.bLabels ? pszState : "");
        snprintf(sc.szLabel2, sizeof(sc.szLabel2), "%s", f.bExists && opt.bLabels ? szLine2 : "");
        g_uSceneNs = NowNs();

        flymod::SStatus& st = g_Status;
        st.bFly      = f.bExists;
        st.eState    = f.bExists ? f.eState : EState::None;
        st.pszState  = pszState;
        st.pszCause  = pszCause;
        for (int i = 0; i < fly::kOutputCount; i++) st.aOut[i] = f.fFilt[i];
        for (int i = 0; i < fly::kInputCount; i++)  st.aIn[i]  = f.aIn[i];
        st.fTurn     = f.fTurn;
        st.fHunger   = f.fHunger;
        st.nDrops    = (int)g_Drops.size();
        st.nEaten    = f.nEaten;
        st.nTakeoffs = f.nTakeoffs;
    }

    std::vector<std::string> BrainPaths(std::string& sPkg)
    {
        char szPkg[128] = {};
        if (FILE* f = fopen("/proc/self/cmdline", "rb"))
        {
            const size_t n = fread(szPkg, 1, sizeof(szPkg) - 1, f);
            szPkg[n] = 0;
            fclose(f);
        }
        if (char* pColon = strchr(szPkg, ':')) *pColon = 0;
        sPkg = szPkg[0] && !strchr(szPkg, '/') ? szPkg : "";
        std::vector<std::string> v;
        if (!sPkg.empty())
        {
            v.push_back("/sdcard/Android/obb/" + sPkg + "/fly_brain.bin");
            v.push_back("/sdcard/Android/data/" + sPkg + "/files/fly_brain.bin");
            v.push_back("/data/data/" + sPkg + "/files/fly_brain.bin");
        }
        v.push_back("/sdcard/Download/fly_brain.bin");
        v.push_back("/sdcard/fly_brain.bin");
        v.push_back("fly_brain.bin");   // тест на ПК
        return v;
    }
}

// ---------------------------------------------------------------- API для меню
void flymod::StartBrain()
{
    static bool s_bStarted = false;   // зовётся каждый кадр страницы
    if (s_bStarted) return;
    s_bStarted = true;
    std::string sPkg;
    fly::SimLoad(BrainPaths(sPkg));
}

std::string flymod::BrainHint()
{
    std::string sPkg;
    BrainPaths(sPkg);
    return "adb push fly_brain.bin /sdcard/Android/obb/" + (sPkg.empty() ? std::string("<game package>") : sPkg) + "/";
}

flymod::SStatus flymod::GetStatus()
{
    std::lock_guard<std::mutex> lock(g_Lock);
    SStatus s = g_Status;
    s.eTool   = g_eTool;
    return s;
}

flymod::SOptions flymod::GetOptions()                 { std::lock_guard<std::mutex> l(g_Lock); return g_Opt; }
void             flymod::SetOptions(const SOptions& o) { std::lock_guard<std::mutex> l(g_Lock); g_Opt = o; }
void             flymod::SetTool(ETool e)             { std::lock_guard<std::mutex> l(g_Lock); g_eTool = e; }
void             flymod::Stimulate(EStim e)           { std::lock_guard<std::mutex> l(g_Lock); g_Cmds.push_back(kCmdStim0 + (int)e); }
void             flymod::RemoveFly()                  { std::lock_guard<std::mutex> l(g_Lock); g_Cmds.push_back(kCmdRemove); }
void             flymod::ClearSugar()                 { std::lock_guard<std::mutex> l(g_Lock); g_Cmds.push_back(kCmdClearSugar); }
void             flymod::ResetBrain()                 { std::lock_guard<std::mutex> l(g_Lock); g_Cmds.push_back(kCmdResetBrain); }

std::string flymod::BeamLabel()
{
    std::lock_guard<std::mutex> lock(g_Lock);
    return g_eTool == ETool::Off ? std::string() : g_sBeam;
}

// ---------------------------------------------------------------- хук
void flymod::SetupWorld() { fly::SetupWorldContext(); }

bool flymod::WorldActive()
{
    std::lock_guard<std::mutex> lock(g_Lock);
    return g_Scene.rig.bValid && (g_Scene.bFly || g_Scene.nDrops > 0) && NowNs() - g_uSceneNs < 500000000ull;
}

void flymod::DrawWorld(const SWorldEye* aEyes, int nEyes)
{
    static fly::SScene s_Scene;   // копия для рендер-потока: рисуем без блокировки
    {
        std::lock_guard<std::mutex> lock(g_Lock);
        s_Scene = g_Scene;
    }
    fly::DrawScene(ImGui::GetBackgroundDrawList(), s_Scene, aEyes, nEyes);
}

// ---------------------------------------------------------------- главный поток
void flymod::Tick(float fDt)
{
    ETool            eTool;
    SOptions         opt;
    std::vector<int> cmds;
    {
        std::lock_guard<std::mutex> lock(g_Lock);
        eTool = g_eTool;
        opt   = g_Opt;
        cmds.swap(g_Cmds);
    }
    fly::SimSetTimeScale(opt.fTimeScale / 100.f);

    SFly& f = g_Fly;
    for (int c : cmds)
    {
        if (c == kCmdRemove)
        {
            if (!f.bExists) { Message("There's no fly"); continue; }
            f = SFly();
            fly::SimRun(false);
            g_bNeedReset = true;
            Message("The fly is gone, its brain is paused");
        }
        else if (c == kCmdClearSugar)
        {
            Message(g_Drops.empty() ? "No sugar to clear" : "Sugar cleared");
            g_Drops.clear();
        }
        else if (c == kCmdResetBrain)
        {
            fly::SimReset();
            for (float& v : f.fFilt) v = 0.f;
            Message("Brain reset: membranes, currents and adaptation are at rest");
        }
        else if (c >= kCmdStim0 && c < kCmdStim0 + 6)
        {
            if (!f.bExists || f.bSwatted) { Message("Spawn a fly first"); continue; }
            f.aStim[c - kCmdStim0] = g_fT;
        }
    }

    // Ни мухи, ни сахара, ни луча — игру не трогаем
    fly::SRig rig;
    const float sNoRig = opt.fSize * 0.001f;
    if (!f.bExists && g_Drops.empty() && eTool == ETool::Off)
    {
        GunOff();
        Publish(rig, opt, sNoRig);
        return;
    }

    Vector3    pos, cam;
    Quaternion rot;
    float      fScale = 1.f;
    if (!ubridge::GetRig(pos, rot, fScale, cam))
    {
        GunOff();
        Publish(rig, opt, sNoRig);
        return;
    }
    rig.bValid = true;
    rig.aPos[0] = pos.x; rig.aPos[1] = pos.y; rig.aPos[2] = pos.z;
    rig.aRot[0] = rot.x; rig.aRot[1] = rot.y; rig.aRot[2] = rot.z; rig.aRot[3] = rot.w;
    rig.fScale  = fScale;
    g_Cam    = cam;
    g_fScale = fScale;
    const float s = opt.fSize * 0.001f * fScale;   // единиц мира в миллиметре мухи

    TickTool(eTool, s);
    TickHands(fDt);

    // Приманка: насколько быстро ползёт точка (LC10 любят движущееся)
    if (g_bLure)
    {
        const float v = fDt > 1e-4f ? Len(Sub(g_Lure, g_LurePrev)) / fDt / s : 0.f;   // мм/с мухи
        g_fLureMotion += (Clamp(v / 15.f, 0.f, 1.f) - g_fLureMotion) * Clamp(fDt / 0.15f, 0.f, 1.f);
    }
    else g_fLureMotion = 0.f;
    g_LurePrev = g_Lure;

    if (f.bExists)
    {
        // Время мухи — сколько прожил мозг: не успевает считать — муха в замедленной съёмке
        float  aHz[fly::kOutputCount];
        double dMs = 0.0;
        float  dt  = 0.f;
        if (f.bSwatted) dt = fDt * opt.fTimeScale / 100.f;
        else if (fly::SimReadout(aHz, dMs))
        {
            dt = (float)(dMs / 1000.0);
            for (int i = 0; i < fly::kOutputCount; i++)
            {
                const float tau = i == fly::kMN9 ? kTauMN9 : (i == fly::kHear2 ? kTauHear : kTauFilt);
                f.fFilt[i] += Clamp(dt / tau, 0.f, 1.f) * (aHz[i] - f.fFilt[i]);
            }
        }
        dt = fminf(dt, 0.1f);
        g_fT += dt;
        Behave(f, s, dt);
        if (f.bSwatted) f.body.Update(dt, fly::SBodyCmd());
    }
    Publish(rig, opt, s);
}
