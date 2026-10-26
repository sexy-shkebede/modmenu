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
    Vector3 V(float x, float y, float z)              { return Vector3(x, y, z); }
    Vector3 Add(const Vector3& a, const Vector3& b)   { return V(a.x + b.x, a.y + b.y, a.z + b.z); }
    Vector3 Sub(const Vector3& a, const Vector3& b)   { return V(a.x - b.x, a.y - b.y, a.z - b.z); }
    Vector3 Mul(const Vector3& a, float s)            { return V(a.x * s, a.y * s, a.z * s); }
    float   Dot(const Vector3& a, const Vector3& b)   { return a.x * b.x + a.y * b.y + a.z * b.z; }
    Vector3 Cross(const Vector3& a, const Vector3& b) { return V(a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x); }
    float   Len(const Vector3& a)                     { return sqrtf(Dot(a, a)); }
    Vector3 Norm(const Vector3& a, const Vector3& fb = V(0.f, 1.f, 0.f)) { const float l = Len(a); return l > 1e-6f ? Mul(a, 1.f / l) : fb; }
    // Часть вектора в плоскости с нормалью n
    Vector3 InPlane(const Vector3& a, const Vector3& n) { return Sub(a, Mul(n, Dot(a, n))); }
    float   Clamp(float v, float lo, float hi)        { return v < lo ? lo : (v > hi ? hi : v); }
    float   Approach(float cur, float target, float rate, float dt) { return cur + (target - cur) * (1.f - expf(-rate * dt)); }
    const Vector3 kUp = V(0.f, 1.f, 0.f);

    uint64_t NowNs()
    {
        timespec ts;
        clock_gettime(CLOCK_MONOTONIC, &ts);
        return (uint64_t)ts.tv_sec * 1000000000ull + (uint64_t)ts.tv_nsec;
    }

    uint32_t g_uRng = 0x9E3779B9u;
    float Rand01() { g_uRng ^= g_uRng << 13; g_uRng ^= g_uRng >> 17; g_uRng ^= g_uRng << 5; return (float)(g_uRng >> 8) / 16777216.f; }

    // ---------------------------------------------------------------- числа из fly-connectome
    // Пороги и сглаживание — tiktok/senses.py (THRESH, Filt); органы чувств и еда — show.py.
    // Расстояния — в мм мухи, время — время мухи (с).
    constexpr float kTauFilt = 0.06f, kTauMN9 = 0.2f, kTauHear = 0.15f;
    constexpr float kTakeoff = 45.f, kStartle = 25.f;
    constexpr float kEatOn = 8.f, kEatKeep = 3.f, kGroomOn = 35.f, kGroomKeep = 12.f;
    constexpr float kTurnGain = 0.12f, kHeadGain = 0.008f;
    constexpr float kGfCool = 1.5f, kStartleCool = 0.4f;
    constexpr float kEatR = 1.7f, kEatTime = 1.4f;
    constexpr float kPuffDur = 1.5f, kPuffTau = 0.8f, kSugarDur = 3.f, kBitterDur = 1.5f, kSoundDur = 0.6f, kLoomDur = 0.4f;
    constexpr float kPulsePer = 0.25f, kPulseOn = 0.10f;   // касания сахара 4 Гц по 100 мс
    constexpr float kLoomMin = 0.2f;                       // ниже — не надвигание (как LOOM_MIN в senses.py)
    // Тело и мир — живая дрозофила (это правила тела, а не мозга)
    constexpr float kWalkMm   = 10.f;    // мм/с при команде ходьбы 1
    constexpr float kTrackMm  = 1.6f;    // "колея" для дифференциального поворота
    constexpr float kCruiseMm = 300.f;   // мм/с: свободный полёт дрозофилы 0.2–0.6 м/с
    constexpr float kEscapeMm = 500.f;   // мм/с: первые 0.4 с взлёта от угрозы
    constexpr float kFlyYawMax = 7.f;    // рад/с: поворот в полёте при руле DNa до упора
    constexpr float kGravity  = 9.81f;   // м/с²: прихлопнутая падает
    constexpr float kDropMm   = 0.9f;    // радиус капли сахара
    constexpr int   kMaxDrops = 8;
    constexpr float kHandR    = 0.045f;  // м: рука с контроллером для глаза мухи
    constexpr float kHeadR    = 0.09f;   // м: голова игрока
    constexpr float kSwatMps  = 0.6f;    // м/с: быстрее — прихлопнул, медленнее — коснулся
    constexpr float kHunger   = 0.01f;   // в секунду: голод растёт (жизнь ускорена, как в show.py)
    constexpr float kDustPuff = 30.f;    // Гц щетинкам при голове в пыли целиком (чистка — от ~15 Гц)
    constexpr float kOdorM    = 1.2f;    // м: запах капли спадает в e раз на таком расстоянии
    constexpr float kSaccade  = 14.f;    // рад/с: резкий разворот в полёте (саккада)

    // Фасетки: 24 направления на глаз (оси мухи), глаз видит вбок и вперёд с перекрытием
    constexpr int kEyeAz = 6, kEyeEl = 4, kFacets = 2 * kEyeAz * kEyeEl;

    enum ECmd { kCmdRemove, kCmdClearSugar, kCmdResetBrain, kCmdStim0 };
    enum class EMode { Walk, Flight, Fall };

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

    enum class ESeek { None, Food, Lure, Hand, Odor };

    struct SFly
    {
        bool      bExists = false, bSwatted = false;
        EMode     eMode = EMode::Walk;
        // поза: на поверхности pos — точка под грудью, up — нормаль поверхности; в полёте up — "верх" тела
        Vector3   pos = V(0.f, 0.f, 0.f), up = kUp, fwd = V(0.f, 0.f, 1.f);
        fly::CBody body;
        // поведение
        EState    eState = EState::Walk;
        ESeek     eSeek = ESeek::None;
        float     fFilt[fly::kOutputCount] = {};
        float     fHunger = 0.7f, fSticky = 0.f, fDust = 0.f, fMn9Last = -9.f, fGfCool = 0.f, fStartleT = -9.f;
        bool      bProb = false;
        float     fTurn = 0.f, fHead = 0.f;
        float     fAvoidT = 0.f, fAvoidSign = 1.f, fRestT = 0.f, fNextRest = 5.f;
        float     fGroundT = 0.f;             // сколько уже на поверхности (с)
        float     fProbeT = 0.f;
        int       iCarrier = -1;              // на какой руке сидит (0 — левая, 1 — правая)
        // полёт
        Vector3   flyDir = V(0.f, 0.f, 1.f);  // курс (горизонтальный)
        Vector3   takeoffVel = V(0.f, 0.f, 0.f);
        float     fFlightT0 = 0.f, fFlightDur = 3.f, fSpeed = 0.f, fVy = 0.f, fYawRate = 0.f;
        float     fTargetAlt = 0.6f;          // м (в масштабе игрока) над полом
        bool      bEscape = false, bLanding = false;
        Vector3   lastGround = V(0.f, 0.f, 0.f);
        float     fNoGroundT = 0.f;
        float     fWallLoom = 0.f;            // надвигание стен в полёте (0..1) — для подписи
        float     fSaccadeT = 0.f, fSaccadeSign = 1.f;
        float     fOdor = 0.f;                // запах еды у антенн (0..1)
        Vector3   odorDir = V(0.f, 0.f, 0.f); // куда сильнее пахнет (мир)
        float     fFoodDist = 1e9f, fFoodDistH = 1e9f;   // до ближней капли (мир): всего и по горизонтали
        // падение
        Vector3   fallVel = V(0.f, 0.f, 0.f);
        float     fSwatT = 0.f;
        bool      bOnBack = false;
        int       nEaten = 0, nFlights = 0;
        // стимулы из меню: когда начались (время мухи), < 0 — нет
        float     aStim[6] = { -1.f, -1.f, -1.f, -1.f, -1.f, -1.f };
        float     fTouchT = -9.f;             // мягкое касание рукой → щетинки
        // руки и голова игрока: угловой размер и как быстро он растёт
        float     aTheta[3] = {}, aDTheta[3] = {};
        bool      aThetaOk[3] = {};
        // фасетки: расстояние до геометрии (мир), < 0 — ничего в пределах дальности
        float     aFacet[kFacets];
        int       iFacetNext = 0;
        float     aIn[fly::kInputCount] = {};
        SFly() { for (float& d : aFacet) d = -1.f; }
    };
    SFly                g_Fly;
    float               g_fT = 0.f;           // время мухи
    std::vector<SDrop>  g_Drops;
    bool                g_bNeedReset = false; // следующей мухе — свежий мозг
    bool                g_bGunOn = false, g_bTriggerDown = false;
    bool                g_bLure = false;
    Vector3             g_Lure = V(0.f, 0.f, 0.f), g_LurePrev = V(0.f, 0.f, 0.f);
    float               g_fLureMotion = 0.f;
    // руки (0, 1) и голова (2) игрока
    Vector3             g_aSrc[3]   = { V(0.f, 0.f, 0.f), V(0.f, 0.f, 0.f), V(0.f, 0.f, 0.f) };
    Vector3             g_aDelta[3] = { V(0.f, 0.f, 0.f), V(0.f, 0.f, 0.f), V(0.f, 0.f, 0.f) };   // смещение за такт
    float               g_aSpeed[3] = {};     // м/с (сглажено)
    bool                g_aSrcOk[3] = {};
    Vector3             g_Cam = V(0.f, 0.f, 0.f);
    float               g_fScale = 1.f;
    Vector3             g_aFacetDir[kFacets]; // направления фасеток в осях мухи

    void InitFacets()
    {
        // Сложный глаз дрозофилы: каждый глаз — почти полусфера сбоку, спереди перекрытие ~20°
        static bool s_bDone = false;
        if (s_bDone) return;
        s_bDone = true;
        const float aAz[kEyeAz] = { -10.f, 20.f, 50.f, 80.f, 115.f, 150.f };   // от "вперёд" наружу
        const float aEl[kEyeEl] = { -50.f, -15.f, 20.f, 55.f };
        int k = 0;
        for (int eye = 0; eye < 2; eye++)
            for (float az : aAz)
                for (float el : aEl)
                {
                    const float a = az * 3.14159265f / 180.f, e = el * 3.14159265f / 180.f;
                    const float side = eye == 0 ? -1.f : 1.f;   // 0 — левый глаз (−X)
                    g_aFacetDir[k++] = V(side * sinf(a) * cosf(e), sinf(e), cosf(a) * cosf(e));
                }
    }

    // ---------------------------------------------------------------- оси мухи
    Vector3 RightOf(const SFly& f) { return Cross(f.up, f.fwd); }
    Vector3 LeftOf(const SFly& f)  { return Cross(f.fwd, f.up); }

    // Точка мухи (мм, оси мухи: x вправо, y вверх, z вперёд) → мир
    Vector3 FlyPoint(const SFly& f, float s, float x, float y, float z)
    {
        return Add(f.pos, Add(Mul(RightOf(f), x * s), Add(Mul(f.up, y * s), Mul(f.fwd, z * s))));
    }
    Vector3 FlyDir(const SFly& f, const Vector3& d) { return Add(Mul(RightOf(f), d.x), Add(Mul(f.up, d.y), Mul(f.fwd, d.z))); }

    void Orthonormalize(SFly& f)
    {
        f.up  = Norm(f.up);
        f.fwd = Norm(InPlane(f.fwd, f.up), Norm(InPlane(V(0.f, 0.f, 1.f), f.up), V(1.f, 0.f, 0.f)));
    }

    // show.py side_split: объект в направлении to → сигнал левому / правому глазу. + угла — влево
    float SideSplit(const SFly& f, const Vector3& to, float fStrength, float fBack, float& l, float& r)
    {
        const float fx = Dot(f.fwd, to), lx = Dot(LeftOf(f), to);
        const float ang = atan2f(lx, fx);
        l = r = 0.f;
        if (fabsf(ang) > fBack) return ang;
        l = fStrength * Clamp(sinf(ang) * 1.2f + 0.25f, 0.f, 1.f);
        r = fStrength * Clamp(-sinf(ang) * 1.2f + 0.25f, 0.f, 1.f);
        return ang;
    }

    // Поверхность под точкой вдоль −up: true — нашлась и примерно так же повёрнута
    bool Probe(const Vector3& at, const Vector3& up, float s, ubridge::SHit& hit, float fAbove = 1.5f, float fBelow = 1.5f)
    {
        if (!ubridge::Raycast(Add(at, Mul(up, fAbove * s)), Mul(up, -1.f), (fAbove + fBelow) * s, hit)) return false;
        return Dot(hit.normal, up) > 0.5f;
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
            g_Fly.fHunger = 0.4f + 0.3f * Rand01();
            if (bReset) fly::SimReset();
            g_bNeedReset = false;
        }
        SFly& f = g_Fly;
        f.bExists  = true;
        f.eMode    = EMode::Walk;
        f.pos      = at;
        f.up       = Norm(normal);
        f.fwd      = Norm(InPlane(Sub(g_Cam, at), f.up), V(0.f, 0.f, 1.f));   // лицом к игроку
        f.iCarrier = -1;
        f.fGroundT = 0.f;
        f.lastGround = at;
        Orthonormalize(f);
        fly::SimRun(true);
        Message(bMove ? "The fly is here now" : "A fly! Its brain is running: 138,639 neurons");
    }

    void TickTool(ETool eTool)
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
            // муха держится на любой поверхности: пол, стена, потолок
            sLabel = !bHit ? "Point at any surface" : (g_Fly.bExists && !g_Fly.bSwatted ? "Move the fly here" : "Spawn a fly here");
            if (bPress)
            {
                if (fly::SimInfo().eLoad != fly::ELoad::Ready) Message("The brain isn't loaded yet: see the Fly page");
                else if (!bHit)                                 Message("A fly needs a surface to sit on");
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
            else if (bPress) Message("A drop runs down a wall: put sugar on a floor or a table");
            break;
        }
        case ETool::Lure:
            g_bLure = bDown && !bOnMenu && bHit;
            sLabel  = g_bLure ? "Lure: move the dot slowly" : "Lure: hold the trigger";
            if (g_bLure) g_Lure = hit.point;
            break;
        default: break;
        }
        std::lock_guard<std::mutex> lock(g_Lock);
        g_sBeam = sLabel;
    }

    // ---------------------------------------------------------------- руки и голова игрока
    void TickPlayer(float fDt)
    {
        COpenXRHook* pHook = g_pOpenXRHook;
        for (int i = 0; i < 3; i++)
        {
            Vector3 p = g_Cam;
            bool    bOk = true;
            if (i < 2)
            {
                XrVector3f dirHead, posHead;
                Vector3    d;
                bOk = pHook && pHook->GetHandInHead(i == 1, false, dirHead, &posHead) &&
                      ubridge::HeadToWorld(V(posHead.x, posHead.y, -posHead.z), V(dirHead.x, dirHead.y, -dirHead.z), p, d);
            }
            g_aDelta[i] = bOk && g_aSrcOk[i] ? Sub(p, g_aSrc[i]) : V(0.f, 0.f, 0.f);
            if (bOk && g_aSrcOk[i] && fDt > 1e-4f)
            {
                const float v = Len(g_aDelta[i]) / fDt / g_fScale;
                g_aSpeed[i] += (v - g_aSpeed[i]) * Clamp(fDt / 0.05f, 0.f, 1.f);
            }
            g_aSrcOk[i] = bOk;
            if (bOk) g_aSrc[i] = p;
        }
    }

    // ---------------------------------------------------------------- взлёт, посадка, падение
    void TakeOff(SFly& f, const Vector3& push, bool bEscape, float s, const char* pszWhy)
    {
        f.eMode     = EMode::Flight;
        f.bEscape   = bEscape;
        f.bLanding  = false;
        f.iCarrier  = -1;
        f.fFlightT0 = g_fT;
        // полёт дрозофилы — секунды: от угрозы короче, голодная ищет дольше
        f.fFlightDur = bEscape ? 0.8f + 1.5f * Rand01() : 1.5f + 3.f * Rand01() + 2.5f * f.fHunger;
        f.fTargetAlt = (0.25f + 1.1f * Rand01()) * g_fScale;
        // толчок ногами от поверхности + рывок: от угрозы — быстро, сам — мягко вверх
        const float v0 = (bEscape ? kEscapeMm : 0.6f * kCruiseMm) * s;
        f.takeoffVel = Mul(Norm(Add(Mul(f.up, bEscape ? 0.8f : 1.2f), push)), v0);
        const Vector3 h = V(f.takeoffVel.x, 0.f, f.takeoffVel.z);
        f.flyDir    = Norm(Len(h) > 1e-4f * s ? h : V(f.fwd.x, 0.f, f.fwd.z), V(0.f, 0.f, 1.f));
        f.fSpeed    = Len(h) / s;
        f.fVy       = f.takeoffVel.y / s;
        f.lastGround = f.pos;
        f.nFlights++;
        if (pszWhy) Message(pszWhy);
    }

    void Land(SFly& f, const ubridge::SHit& hit)
    {
        const Vector3 dir = f.flyDir;
        f.eMode    = EMode::Walk;
        f.pos      = hit.point;
        f.up       = Norm(hit.normal);
        // на стене садится головой вверх, на полу — по курсу
        f.fwd      = fabsf(f.up.y) < 0.6f ? InPlane(kUp, f.up) : InPlane(dir, f.up);
        Orthonormalize(f);
        f.fGroundT = 0.f;
        f.fGfCool  = kGfCool;
        f.fDust    = fminf(1.f, f.fDust + 0.1f);   // после посадки мухи чистятся
        f.lastGround = f.pos;
        f.fAvoidT  = 0.f;
    }

    void Swat(SFly& f)
    {
        f.bSwatted = true;
        f.fSwatT   = g_fT;
        f.eState   = EState::Swatted;
        f.fallVel  = f.eMode == EMode::Flight ? Add(Mul(f.flyDir, f.fSpeed), V(0.f, f.fVy, 0.f)) : V(0.f, 0.f, 0.f);
        // в полёте, на стене или на потолке — падает; на полу — сразу на спину
        f.eMode    = f.eMode == EMode::Flight || f.up.y < 0.7f ? EMode::Fall : EMode::Walk;
        f.bOnBack  = f.eMode == EMode::Walk;
        f.iCarrier = -1;
        fly::SimRun(false);              // мёртвый мозг гаснет
        Message("Swatted: it didn't get away in time. Spawn a new fly with the beam");
    }

    void TickFall(SFly& f, float s, float dt)
    {
        // скорость — в мм мухи; падение — по g мира
        f.fallVel.y -= kGravity * g_fScale / s * dt;
        const Vector3 step = Mul(f.fallVel, s * dt);
        const float   l = Len(step);
        ubridge::SHit hit {};
        if (l > 1e-7f && ubridge::Raycast(f.pos, Mul(step, 1.f / l), l + 0.3f * s, hit))
        {
            f.pos = hit.point;
            f.up  = Norm(hit.normal);
            Orthonormalize(f);
            f.eMode   = EMode::Walk;
            f.bOnBack = true;
            f.fSwatT  = g_fT;
            return;
        }
        f.pos = Add(f.pos, step);
        // кувыркается в воздухе
        const float a = 9.f * dt;
        f.up = Norm(Add(Mul(f.up, cosf(a)), Mul(RightOf(f), sinf(a))));
        Orthonormalize(f);
    }

    // ---------------------------------------------------------------- глаза: геометрия игры
    // Фасетки — лучи из головы. В полёте надвигание стен (скорость сближения / расстояние, как
    // величина, обратная времени до столкновения) идёт в LPLC2 / LC4 своего глаза.
    void TickEye(SFly& f, float s, const Vector3& vel, float& lL, float& lR)
    {
        lL = lR = 0.f;
        const Vector3 head  = FlyPoint(f, s, 0.f, 0.66f, 1.02f);
        const float   range = kCruiseMm * 0.6f * s;
        for (int n = 0; n < kFacets / 2; n++)   // половина фасеток за такт, по кругу
        {
            const int k = f.iFacetNext;
            f.iFacetNext = (f.iFacetNext + 1) % kFacets;
            ubridge::SHit hit {};
            f.aFacet[k] = ubridge::Raycast(head, FlyDir(f, g_aFacetDir[k]), range, hit) ? hit.fDistance : -1.f;
        }
        for (int k = 0; k < kFacets; k++)
        {
            if (f.aFacet[k] <= 0.f) continue;
            const float closing = Dot(vel, FlyDir(f, g_aFacetDir[k]));   // мир/с
            if (closing <= 0.f) continue;
            const float e    = closing / fmaxf(f.aFacet[k], 0.5f * s);    // 1 / время до столкновения
            const float loom = Clamp((e - 3.f) / 7.f, 0.f, 1.f);
            if (k < kFacets / 2) lL = fmaxf(lL, loom); else lR = fmaxf(lR, loom);
        }
    }

    // ---------------------------------------------------------------- органы чувств
    struct SSense { bool bTouching = false; SDrop* pNear = nullptr; Vector3 threat = V(0.f, 0.f, 0.f); float fThreat = 0.f; float fFood = 0.f; };

    // Запах капель: концентрация у точки (0..~1) и направление, где она растёт
    float Odor(const Vector3& p, Vector3* pGrad)
    {
        float c = 0.f;
        Vector3 g = V(0.f, 0.f, 0.f);
        const float lambda = kOdorM * g_fScale;
        for (const SDrop& d : g_Drops)
        {
            const Vector3 to = Sub(d.p, p);
            const float   l  = Len(to);
            const float   k  = d.fLeft * expf(-l / lambda);
            c += k;
            if (l > 1e-5f) g = Add(g, Mul(to, k / l));
        }
        if (pGrad) *pGrad = g;
        return c;
    }

    void Senses(SFly& f, float s, float dt, const Vector3& vel, SSense& out)
    {
        float* in = f.aIn;
        for (int i = 0; i < fly::kInputCount; i++) in[i] = 0.f;
        const Vector3 head = FlyPoint(f, s, 0.f, 0.66f, 1.02f);
        const bool    bFlying = f.eMode == EMode::Flight;

        // ---- мелкие движущиеся предметы и еда → LC10 (show.py: сила 6 / (d + 2), d в мм)
        float fL = 0.f, fR = 0.f, fBest = 0.f;
        f.eSeek = ESeek::None;
        const auto Object = [&](const Vector3& to, float fStrength, ESeek eKind)
        {
            float l, r;
            SideSplit(f, to, fStrength, 3.14159265f * 165.f / 180.f, l, r);
            fL += l; fR += r;
            if (l + r > fBest && l + r > 0.05f) { fBest = l + r; f.eSeek = eKind; }
            return l + r;
        };
        out.fFood = 0.f;
        for (SDrop& d : g_Drops)
        {
            const Vector3 to = Sub(d.p, head);
            const float   lr = Object(to, Clamp(6.f / (Len(to) / s + 2.f), 0.f, 1.f) * d.fLeft, ESeek::Food);
            out.fFood = fmaxf(out.fFood, lr);
            if (lr > 0.3f && !d.bSeen && f.eState != EState::Eat)
            {
                d.bSeen = true;
                Message("Sugar spotted: LC10 steers the fly through DNa02");
            }
        }
        if (g_bLure)
            Object(Sub(g_Lure, head), Clamp(6.f / (Len(Sub(g_Lure, head)) / s + 2.f), 0.f, 1.f) * (0.3f + 0.7f * g_fLureMotion), ESeek::Lure);
        // руки и голова игрока, пока они малы в поле зрения и движутся (LC10 любят как раз такое)
        for (int k = 0; k < 3; k++)
        {
            if (!g_aSrcOk[k] || k == f.iCarrier || dt <= 0.f) continue;
            const Vector3 to   = Sub(g_aSrc[k], head);
            const float   dist = Len(to);
            const float   R    = (k < 2 ? kHandR : kHeadR) * g_fScale;
            if (dist < R * 1.5f) continue;
            const float   th   = 2.f * atanf(R / dist);
            const Vector3 u    = Mul(to, 1.f / dist);
            // своё движение предмета (мир/с): когда летит сама муха, сдвигается весь мир разом —
            // это не "движущийся предмет" для LC10
            const Vector3 own  = Mul(g_aDelta[k], 1.f / fmaxf(dt, 1e-3f));
            const float   ang  = Len(InPlane(own, u)) / dist;                          // рад/с поперёк взгляда
            if (th < 0.35f && ang > 0.4f)
                Object(to, 0.6f * Clamp(ang / 2.5f, 0.f, 1.f) * Clamp((0.35f - th) / 0.2f, 0.f, 1.f), ESeek::Hand);
        }
        in[fly::kObjL] = 60.f * fminf(fL, 1.f);
        in[fly::kObjR] = 60.f * fminf(fR, 1.f);

        // ---- запах: обонятельные рецепторы левой и правой антенны (у каждой — свой воздух)
        const float cL = Odor(FlyPoint(f, s, -0.15f, 0.8f, 1.3f), nullptr), cR = Odor(FlyPoint(f, s, 0.15f, 0.8f, 1.3f), &f.odorDir);
        in[fly::kOdorL] = 70.f * cL / (cL + 0.25f);
        in[fly::kOdorR] = 70.f * cR / (cR + 0.25f);
        f.fOdor = Clamp(0.5f * (cL + cR), 0.f, 1.f);
        f.fFoodDist = f.fFoodDistH = 1e9f;
        for (const SDrop& d : g_Drops)
        {
            f.fFoodDist  = fminf(f.fFoodDist, Len(Sub(d.p, head)));
            f.fFoodDistH = fminf(f.fFoodDistH, Len(InPlane(Sub(d.p, head), kUp)));
        }
        if (f.eSeek == ESeek::None && f.fOdor > 0.03f) f.eSeek = ESeek::Odor;

        // ---- надвигание → LPLC2 / LC4. Руки и голова игрока: θ = 2 atan(R / d), сигнал — пока θ
        // растёт (show.py: θ / 0.9, тут ещё и по скорости роста). В полёте — ещё и стены (глаза)
        float tL = 0.f, tR = 0.f;
        out.fThreat = 0.f;
        for (int k = 0; k < 3; k++)
        {
            if (!g_aSrcOk[k] || k == f.iCarrier) { f.aThetaOk[k] = false; continue; }
            const float   R    = (k < 2 ? kHandR : kHeadR) * g_fScale;
            const Vector3 to   = Sub(g_aSrc[k], head);
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
            SideSplit(f, to, lv, 3.14159265f * 175.f / 180.f, l, r);
            tL = fmaxf(tL, l); tR = fmaxf(tR, r);
            if (lv > out.fThreat) { out.fThreat = lv; out.threat = g_aSrc[k]; }
        }
        f.fWallLoom = 0.f;
        if (bFlying)
        {
            float eL, eR;
            TickEye(f, s, vel, eL, eR);
            f.fWallLoom = fmaxf(eL, eR);
            tL = fmaxf(tL, Clamp((eL - kLoomMin) / (1.f - kLoomMin), 0.f, 1.f));
            tR = fmaxf(tR, Clamp((eR - kLoomMin) / (1.f - kLoomMin), 0.f, 1.f));
        }
        in[fly::kLoomL] = 80.f * tL;
        in[fly::kLoomR] = 80.f * tR;

        // ---- касание руки у груди: медленно — щетинки (чистка), быстро — прихлопнул
        const Vector3 thorax = FlyPoint(f, s, 0.f, 0.62f, 0.35f);
        for (int k = 0; k < 2; k++)
        {
            if (!g_aSrcOk[k] || k == f.iCarrier || f.bSwatted) continue;
            if (Len(Sub(g_aSrc[k], thorax)) > kHandR * g_fScale * 0.6f + 1.4f * s) continue;
            if (g_aSpeed[k] > kSwatMps) { Swat(f); return; }
            if (!bFlying && g_fT - f.fTouchT > kPuffDur)
            {
                f.fTouchT = g_fT;
                Message("Touched: head bristles fire, grooming neurons listen");
            }
        }

        // ---- еда под головой → сахарные нейроны (show.py: 80 + 120 × голод)
        const float rEat = kEatR + (f.eState == EState::Eat ? 0.7f : 0.f);
        out.pNear = nullptr;
        float best = 1e9f;
        if (!bFlying)
            for (SDrop& d : g_Drops)
            {
                const float dist = Len(InPlane(Sub(d.p, head), f.up)) / s;
                if (dist < rEat && dist < best && fabsf(Dot(Sub(d.p, head), f.up)) < 3.f * s) { best = dist; out.pNear = &d; }
            }
        out.bTouching = out.pNear != nullptr;
        if (out.bTouching) in[fly::kSugar] = 80.f + 120.f * f.fHunger;

        // ---- щетинки головы: пыль, липкий сироп после еды (show.py: 90 × sticky), касание, стимул Puff
        float puff = fmaxf(kDustPuff * f.fDust, f.fSticky > 0.05f ? 90.f * f.fSticky : 0.f), wind = 0.f;
        const float aPuffAge[2] = { StimAge(f, EStim::Puff), g_fT - f.fTouchT };
        for (float tp : aPuffAge)
            if (tp >= 0.f && tp < kPuffDur)
            {
                puff = fmaxf(puff, 60.f * expf(-tp / kPuffTau));
                wind = fmaxf(wind, 40.f * expf(-tp / kPuffTau));
            }
        // встречный поток в полёте отклоняет антенны — Джонстонов орган, зоны C/E
        if (bFlying) wind = fmaxf(wind, 20.f * Clamp(f.fSpeed / kCruiseMm, 0.f, 1.5f));
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

    // ---------------------------------------------------------------- шаг по поверхности
    // Муха держится лапками за что угодно: впереди стена — залезает на неё, край — огибает его
    // и идёт по боковой грани, потолок — ходит вниз головой. false — дальше пути нет.
    bool Walk(SFly& f, float s, float v, float w, float dt)
    {
        const float a = w * dt;
        f.fwd = Norm(Add(Mul(f.fwd, cosf(a)), Mul(LeftOf(f), sinf(a))));
        if (v <= 1e-4f) return true;
        const float   step = v * dt;
        ubridge::SHit hit {};

        // стена впереди (вогнутый угол): забраться — вперёд теперь "вверх" по ней
        if (ubridge::Raycast(Add(f.pos, Mul(f.up, 0.5f * s)), f.fwd, (1.4f + step) * s, hit) && Dot(hit.normal, f.up) < 0.7f)
        {
            const Vector3 upOld = f.up;
            f.pos = hit.point;
            f.up  = Norm(hit.normal);
            f.fwd = Norm(InPlane(upOld, f.up), f.fwd);
            Orthonormalize(f);
            return true;
        }
        const Vector3 next = Add(f.pos, Mul(f.fwd, step * s));
        ubridge::SHit head {};
        if (Probe(Add(next, Mul(f.fwd, 1.f * s)), f.up, s, head) && Probe(next, f.up, s, hit))
        {
            f.pos = hit.point;
            f.up  = Norm(Add(Mul(f.up, 0.7f), Mul(hit.normal, 0.3f)));
            Orthonormalize(f);
            return true;
        }
        // край (выпуклый угол): под краем ищем боковую грань и переходим на неё
        const Vector3 under = Sub(Add(next, Mul(f.fwd, 2.f * s)), Mul(f.up, 1.f * s));
        if (ubridge::Raycast(under, Mul(f.fwd, -1.f), 3.f * s, hit) && Dot(hit.normal, f.fwd) > 0.5f)
        {
            const Vector3 upOld = f.up;
            f.pos = hit.point;
            f.up  = Norm(hit.normal);
            f.fwd = Norm(InPlane(Mul(upOld, -1.f), f.up), f.fwd);
            Orthonormalize(f);
            return true;
        }
        return false;
    }

    // ---------------------------------------------------------------- полёт
    // Курс ведёт мозг (руль DNa01/DNa02, к еде через LC10, от надвигания через LPLC2/LC4). Тягу,
    // высоту и посадку крыльями и галтерами держит тело: этих центров в модели мозга нет.
    void Fly(SFly& f, float s, float dt, fly::SBodyCmd& cmd, float fFood)
    {
        const float age = g_fT - f.fFlightT0;
        cmd.bFlight = true;

        // ---- руль
        f.fTurn = tanhf(kTurnGain * (f.fFilt[fly::kSteerL] - f.fFilt[fly::kSteerR]));
        float w = kFlyYawMax * f.fTurn;

        // ---- пол и потолок
        ubridge::SHit hit {};
        float h = 1e9f;
        if (ubridge::Raycast(f.pos, V(0.f, -1.f, 0.f), 3.f * g_fScale, hit) && hit.normal.y > 0.3f)
        {
            h = f.pos.y - hit.point.y;
            f.lastGround = hit.point;
            f.fNoGroundT = 0.f;
        }
        else f.fNoGroundT += dt;
        const bool bCeiling = ubridge::Raycast(f.pos, kUp, 0.15f * g_fScale + 3.f * s, hit);

        // ---- запах еды: лететь туда, где он сильнее (правило тела: в модели мозга нет пути
        // от обоняния к рулю — проверено, DNa на запах не отвечают)
        if (f.fOdor > 0.03f && Len(InPlane(f.odorDir, kUp)) > 1e-6f)
        {
            const Vector3 g   = InPlane(f.odorDir, kUp);
            const float   ang = atan2f(Dot(Cross(f.flyDir, kUp), g), Dot(f.flyDir, g));
            w += Clamp(ang * 2.5f, -4.f, 4.f) * (0.3f + 0.7f * f.fHunger) * Clamp(f.fOdor / 0.1f, 0.f, 1.f);
        }
        // ---- когда садиться: налеталась, еда рядом (видна или пахнет вот тут)
        const bool bFoodNear = f.fFoodDistH < 0.3f * g_fScale && f.fHunger > 0.15f;
        if (!f.bLanding && (age > f.fFlightDur || ((fFood > 0.25f || bFoodNear) && age > 0.6f))) f.bLanding = true;
        // улетела туда, где нет земли — назад, к последнему полу (правило тела)
        if (f.fNoGroundT > 1.5f)
        {
            const Vector3 back = Sub(f.lastGround, f.pos);
            const float   ang  = atan2f(Dot(Cross(f.flyDir, kUp), back), Dot(f.flyDir, back));
            w += Clamp(ang * 3.f, -kFlyYawMax, kFlyYawMax);
        }

        // ---- скорость и высота
        const float vTarget = f.bEscape && age < 0.4f ? kEscapeMm : (f.bLanding ? 0.5f * kCruiseMm : kCruiseMm);
        f.fSpeed = Approach(f.fSpeed, vTarget, 4.f, dt);
        float vyTarget;
        if (f.bLanding)                 vyTarget = -0.35f * f.fSpeed;
        else if (h < 1e8f)              vyTarget = Clamp((f.fTargetAlt - h) / s * 2.5f, -0.45f * f.fSpeed, 0.45f * f.fSpeed);
        else                            vyTarget = Clamp((f.lastGround.y + f.fTargetAlt - f.pos.y) / s * 2.5f, -0.45f * f.fSpeed, 0.45f * f.fSpeed);
        if (bCeiling) vyTarget = fminf(vyTarget, -0.3f * f.fSpeed);
        f.fVy = Approach(f.fVy, vyTarget, 3.f, dt);

        // скорость мира: первые 0.3 с — толчок взлёта, дальше — управляемый полёт
        Vector3 vel = Add(Mul(f.flyDir, f.fSpeed * s), V(0.f, f.fVy * s, 0.f));
        const float kick = Clamp(1.f - age / 0.3f, 0.f, 1.f);
        vel = Add(Mul(vel, 1.f - kick), Mul(f.takeoffVel, kick));

        // ---- что впереди по курсу. Пора садиться — садится (на пол, стену, потолок). Нет —
        // уходит саккадой: резкий разворот от препятствия, как у живых мух. Не успеть (меньше
        // 60 мс до удара) — врезается и садится как есть
        const float speed = Len(vel);
        if (speed > 1e-5f)
        {
            const Vector3 dir = Mul(vel, 1.f / speed);
            if (ubridge::Raycast(f.pos, dir, speed * (0.2f + dt) + 1.f * s, hit))
            {
                const float ttc = hit.fDistance / speed;
                if ((f.bLanding && age > 0.3f) || ttc < 0.06f) { Land(f, hit); return; }
                if (f.fSaccadeT <= 0.f)
                {
                    f.fSaccadeT    = 0.12f + 0.08f * Rand01();
                    f.fSaccadeSign = Dot(hit.normal, Cross(f.flyDir, kUp)) > 0.f ? 1.f : -1.f;   // туда, куда смотрит стена
                }
            }
        }
        if (f.fSaccadeT > 0.f)
        {
            f.fSaccadeT -= dt;
            w = f.fSaccadeSign * kSaccade;
            f.fSpeed *= expf(-2.f * dt);
        }
        if (f.bLanding && h < 1.5f * s + fabsf(f.fVy) * s * dt)
        {
            if (ubridge::Raycast(f.pos, V(0.f, -1.f, 0.f), 3.f * s + fabsf(f.fVy) * s * dt, hit)) { Land(f, hit); return; }
        }

        // ---- движение
        const Vector3 left = Cross(f.flyDir, kUp);
        f.flyDir  = Norm(Add(Mul(f.flyDir, cosf(w * dt)), Mul(left, sinf(w * dt))), f.flyDir);
        f.fYawRate = w;
        f.pos     = Add(f.pos, Mul(vel, dt));

        // поза тела: нос чуть вверх (тем больше, чем медленнее), крен в сторону поворота
        const float pitch = 0.45f - 0.3f * Clamp(f.fSpeed / kCruiseMm, 0.f, 1.5f) + 0.5f * atan2f(f.fVy, fmaxf(f.fSpeed, 1.f));
        const float bank  = Clamp(0.08f * w, -0.7f, 0.7f);
        f.fwd = Norm(Add(Mul(f.flyDir, cosf(pitch)), Mul(kUp, sinf(pitch))));
        f.up  = Norm(Add(Mul(Sub(kUp, Mul(f.fwd, Dot(kUp, f.fwd))), cosf(bank)), Mul(left, sinf(bank))));
        Orthonormalize(f);
    }

    // ---------------------------------------------------------------- поведение
    void Behave(SFly& f, float s, float dt, float fWallDt)
    {
        if (f.eMode == EMode::Fall) { TickFall(f, s, dt); return; }

        // скорость мухи для глаз: в полёте — своя, на руке — рука
        Vector3 vel = V(0.f, 0.f, 0.f);
        if (f.eMode == EMode::Flight) vel = Add(Mul(f.flyDir, f.fSpeed * s), V(0.f, f.fVy * s, 0.f));
        SSense se;
        Senses(f, s, dt, vel, se);
        if (f.bSwatted) return;
        fly::SimSetInputs(f.aIn);

        const float* F  = f.fFilt;
        const float  gf = F[fly::kGF], mn9 = F[fly::kMN9], groom = F[fly::kGroom];
        f.fHunger = fminf(1.f, f.fHunger + kHunger * dt);
        f.fSticky *= expf(-dt / 0.8f);
        f.fDust    = fminf(1.f, f.fDust + dt * (f.eMode == EMode::Flight ? 0.03f : 0.012f));
        f.fGfCool -= dt;
        if (mn9 > 3.f) f.fMn9Last = g_fT;

        fly::SBodyCmd cmd;
        const bool bProb = mn9 > kEatOn || (f.bProb && mn9 > kEatKeep);
        f.bProb = bProb;
        cmd.fProboscis = bProb ? Clamp(mn9 / 12.f, 0.f, 1.f) : 0.f;
        f.fHead  = tanhf(kHeadGain * (F[fly::kHeadL] - F[fly::kHeadR]));
        cmd.fHead = f.fHead;

        if (f.eMode == EMode::Flight)
        {
            Fly(f, s, dt, cmd, f.eSeek == ESeek::Food ? se.fFood : 0.f);
            f.eState = f.eMode != EMode::Flight ? EState::Walk
                     : g_fT - f.fFlightT0 < 0.4f ? EState::Takeoff
                     : f.fWallLoom > 0.3f        ? EState::Dodge
                     : f.bLanding                ? EState::Landing
                     : f.eSeek != ESeek::None    ? EState::Seek : EState::Flight;
            f.body.Update(dt, cmd);
            return;
        }

        // ---------------- на поверхности
        f.fGroundT += dt;
        // сидит на руке игрока — едет вместе с ней
        if (f.iCarrier >= 0 && g_aSrcOk[f.iCarrier]) f.pos = Add(f.pos, g_aDelta[f.iCarrier]);
        f.iCarrier = -1;
        for (int k = 0; k < 2; k++)
            if (g_aSrcOk[k] && Len(Sub(f.pos, g_aSrc[k])) < 0.09f * g_fScale) f.iCarrier = k;
        // опора пропала (подвинули, убрали) — улетает
        f.fProbeT -= fWallDt;
        if (f.fProbeT <= 0.f)
        {
            f.fProbeT = 0.2f;
            ubridge::SHit hit {};
            if (!Probe(f.pos, f.up, s, hit, 1.f, 2.f))
            {
                TakeOff(f, V(0.f, 0.f, 0.f), false, s, "Lost its footing and took off");
                f.body.Update(dt, cmd);
                return;
            }
        }

        // ---- взлёт от угрозы: гигантское волокно DNp01
        if (gf > kTakeoff && f.fGfCool <= 0.f)
        {
            Vector3 away;
            if (se.fThreat > 0.f) away = Sub(f.pos, se.threat);
            else
            {
                // угроза из меню (надвигание слева / справа) — от неё; иначе вперёд
                const float side = f.aIn[fly::kLoomL] - f.aIn[fly::kLoomR];
                away = fabsf(side) > 1.f ? Mul(LeftOf(f), side > 0.f ? -1.f : 1.f) : f.fwd;
            }
            TakeOff(f, Norm(InPlane(away, f.up), f.fwd), true, s, "Takeoff! The giant fiber DNp01 fired");
            f.body.Update(dt, cmd);
            return;
        }
        if (gf > kStartle && gf <= kTakeoff && g_fT - f.fStartleT > kStartleCool) f.fStartleT = g_fT;
        cmd.bStartle = g_fT - f.fStartleT < 0.12f;

        const bool bGroom = !se.bTouching && (groom > kGroomOn || (f.eState == EState::Groom && groom > kGroomKeep));
        const bool bEat   = se.bTouching && (mn9 > kEatOn || (f.eState == EState::Eat && g_fT - f.fMn9Last < 0.8f));
        if (bGroom)
        {
            f.eState = EState::Groom;
            cmd.bGroom = true;
            f.fDust = fmaxf(0.f, f.fDust - 0.25f * dt);   // чистка стряхивает пыль
        }
        else if (bEat)
        {
            // У капли: подползти, пока хоботок до неё не достаёт (правило тела)
            f.eState = EState::Eat;
            cmd.bEat = true;
            const Vector3 to   = InPlane(Sub(se.pNear->p, FlyPoint(f, s, 0.f, 0.66f, 1.02f)), f.up);
            const float   dist = Len(to) / s, reach = kDropMm * (0.35f + 0.65f * se.pNear->fLeft) * 0.5f + 0.2f;
            if (dist > reach)
            {
                const float ang = atan2f(Dot(LeftOf(f), to), Dot(f.fwd, to));
                cmd.fFwd = fminf(1.5f, (dist - reach) * 4.f) * Clamp(cosf(ang), 0.f, 1.f);
                cmd.fYaw = Clamp(ang * 3.f, -2.f, 2.f);
                Walk(f, s, cmd.fFwd, cmd.fYaw, dt);
            }
        }
        else if (bProb) f.eState = EState::Proboscis;
        else
        {
            // ---- ходьба: руль DNa01/DNa02 → дифференциальная команда ногам (show.py)
            f.fTurn = tanhf(kTurnGain * (F[fly::kSteerL] - F[fly::kSteerR]));
            float aL = Clamp(1.f - fmaxf(f.fTurn, 0.f), 0.f, 1.2f), aR = Clamp(1.f + fminf(f.fTurn, 0.f), 0.f, 1.2f);
            const bool bSeek   = f.aIn[fly::kObjL] + f.aIn[fly::kObjR] > 3.f;
            const bool bThreat = fmaxf(f.aIn[fly::kLoomL], f.aIn[fly::kLoomR]) > 0.15f * 80.f;
            f.eState = bThreat ? EState::Threat : (bSeek ? EState::Seek : EState::Walk);

            // паузы, когда идти некуда (правило тела: у живых мух ходьба — сериями)
            if (!bSeek && !bThreat)
            {
                if (f.fRestT > 0.f) { f.fRestT -= dt; aL = aR = 0.f; f.eState = EState::Rest; }
                else if ((f.fNextRest -= dt) <= 0.f)
                {
                    f.fRestT    = 0.8f + 2.f * Rand01();
                    f.fNextRest = 4.f + 6.f * Rand01();
                }
            }
            float v = kWalkMm * (aL + aR) * 0.5f;
            float w = (aR - aL) * kWalkMm / kTrackMm * 0.5f;
            // запах еды: идти туда, где он сильнее (правило тела, как в полёте)
            if (!bThreat && f.fOdor > 0.03f && f.fHunger > 0.1f)
            {
                const Vector3 g = InPlane(f.odorDir, f.up);
                if (Len(g) > 1e-6f)
                {
                    const float ang = atan2f(Dot(LeftOf(f), g), Dot(f.fwd, g));
                    w += Clamp(ang * 1.5f, -2.f, 2.f) * (0.3f + 0.7f * f.fHunger) * Clamp(f.fOdor / 0.1f, 0.f, 1.f);
                    if (f.eState == EState::Rest) { f.fRestT = 0.f; aL = aR = 1.f; v = kWalkMm; }
                    if (f.eState == EState::Walk || f.eState == EState::Rest) f.eState = EState::Seek;
                }
            }
            if (f.fAvoidT > 0.f)
            {
                f.fAvoidT -= dt;
                v = kWalkMm * 0.25f;
                w = f.fAvoidSign * 3.f;
                f.eState = EState::Stuck;
            }
            cmd.fFwd = v;
            cmd.fYaw = w;
            if (!Walk(f, s, v, w, dt) && f.fAvoidT <= 0.f)
            {
                f.fAvoidT    = 0.6f;
                f.fAvoidSign = Rand01() < 0.5f ? -1.f : 1.f;
            }

            // ---- сама взлетает: голодная ищет еду, сытая — реже (в модели нет нейронов
            // "решения взлететь", кроме побега, — это правило тела, а частота зависит от голода)
            // пахнет едой, но она далеко — голодная скорее полетит туда
            const bool  bFar = f.fOdor > 0.03f && f.fFoodDist > 0.6f * g_fScale;
            const float rate = se.fFood > 0.05f ? 0.f : 0.015f + 0.1f * f.fHunger * f.fHunger + (bFar ? 0.35f * f.fHunger : 0.f);
            if (f.fGroundT > 3.f && f.eState != EState::Groom && Rand01() < rate * dt)
                TakeOff(f, Mul(f.fwd, 0.4f), false, s, f.fHunger > 0.6f ? "Hungry: flies off to search for food" : "Took off");
        }

        // ---- еда съедается, пока работает хоботок (show.py)
        if (f.eState == EState::Eat && se.pNear && mn9 > 3.f)
        {
            se.pNear->fLeft = fmaxf(0.f, se.pNear->fLeft - dt / kEatTime);
            f.fHunger       = fmaxf(0.f, f.fHunger - 0.35f * dt / kEatTime);
            if (se.pNear->fLeft <= 0.f)
            {
                g_Drops.erase(g_Drops.begin() + (se.pNear - g_Drops.data()));
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
        const bool bStartle = g_fT - f.fStartleT < 0.6f && f.eMode == EMode::Walk && !f.bSwatted;
        if (bStartle)
        {
            pszState = "Startle"; pszCause = "DNp01 giant fiber";
            snprintf(szLine2, n, "DNp01 giant fiber · %.0f Hz", F[fly::kGF]);
            return;
        }
        const char* pszSeek = f.eSeek == ESeek::Food || f.eSeek == ESeek::Odor ? "food" : (f.eSeek == ESeek::Lure ? "the lure" : "your hand");
        static char s_szSeek[48];
        switch (f.eState)
        {
        case EState::Swatted:   pszState = "Swatted";         pszCause = "didn't get away in time"; break;
        case EState::Takeoff:   pszState = "Takeoff!";        pszCause = f.bEscape ? "DNp01 giant fiber" : "body rule: decided to fly";
                                if (f.bEscape) { snprintf(szLine2, n, "DNp01 giant fiber · %.0f Hz", F[fly::kGF]); return; }
                                break;
        case EState::Dodge:     pszState = "Dodging";         pszCause = "LPLC2 / LC4 → DNa01 / DNa02";
                                snprintf(szLine2, n, "wall looming → DNa L %.0f · R %.0f Hz", F[fly::kSteerL], F[fly::kSteerR]); return;
        case EState::Landing:   pszState = "Landing";         pszCause = "body rule: landing"; break;
        case EState::Flight:    pszState = fabsf(f.fTurn) > 0.3f ? (f.fTurn > 0.f ? "Flying, turning left" : "Flying, turning right") : "Flying";
                                pszCause = "DNa01 / DNa02 steering";
                                snprintf(szLine2, n, "DNa L %.0f · R %.0f Hz", F[fly::kSteerL], F[fly::kSteerR]); return;
        case EState::Groom:     pszState = "Grooming";        pszCause = "DNg84 / DNg35";
                                snprintf(szLine2, n, "DNg84 / DNg35 · %.0f Hz", F[fly::kGroom]); return;
        case EState::Eat:       pszState = "Drinking sugar";  pszCause = "MN9 proboscis motor neuron";
                                snprintf(szLine2, n, "MN9 proboscis · %.0f Hz", F[fly::kMN9]); return;
        case EState::Proboscis: pszState = "Proboscis out";   pszCause = "MN9 proboscis motor neuron";
                                snprintf(szLine2, n, "MN9 proboscis · %.0f Hz", F[fly::kMN9]); return;
        case EState::Threat:    pszState = "Threat!";         pszCause = "LPLC2 / LC4 looming";
                                snprintf(szLine2, n, "LPLC2 / LC4 → giant fiber %.0f Hz", F[fly::kGF]); return;
        case EState::Seek:      if (f.eSeek == ESeek::Odor)
                                {
                                    pszState = f.eMode == EMode::Flight ? "Flying to a smell" : "Smelling food";
                                    pszCause = "body rule: up the odor gradient";
                                    snprintf(szLine2, n, "smell: ORN %.0f Hz → body rule", 0.5f * (f.aIn[fly::kOdorL] + f.aIn[fly::kOdorR]));
                                    return;
                                }
                                snprintf(s_szSeek, sizeof(s_szSeek), "%s %s", f.eMode == EMode::Flight ? "Flying to" : (f.eSeek == ESeek::Hand ? "Watching" : "Going for"), pszSeek);
                                pszState = s_szSeek;
                                pszCause = "LC10 → DNa01 / DNa02";
                                snprintf(szLine2, n, "LC10 → DNa L %.0f · R %.0f Hz", F[fly::kSteerL], F[fly::kSteerR]); return;
        case EState::Rest:      pszState = "Resting";         pszCause = "body rule: a pause"; break;
        case EState::Stuck:     pszState = "Turning back";    pszCause = "body rule: no surface ahead"; break;
        default:                pszState = fabsf(f.fTurn) > 0.3f ? (f.fTurn > 0.f ? "Turning left" : "Turning right") : "Walking";
                                pszCause = fabsf(f.fTurn) > 0.3f ? "DNa01 / DNa02" : "walking rhythm of the body";
                                if (fabsf(f.fTurn) > 0.3f) { snprintf(szLine2, n, "DNa L %.0f · R %.0f Hz", F[fly::kSteerL], F[fly::kSteerR]); return; }
                                break;
        }
        snprintf(szLine2, n, "%s", pszCause);
    }

    void QuatFromBasis(const Vector3& X, const Vector3& Y, const Vector3& Z, float q[4])
    {
        // столбцы матрицы поворота — куда переходят оси мухи
        const float m00 = X.x, m10 = X.y, m20 = X.z, m01 = Y.x, m11 = Y.y, m21 = Y.z, m02 = Z.x, m12 = Z.y, m22 = Z.z;
        const float tr = m00 + m11 + m22;
        if (tr > 0.f)        { const float k = sqrtf(tr + 1.f) * 2.f;             q[3] = 0.25f * k; q[0] = (m21 - m12) / k; q[1] = (m02 - m20) / k; q[2] = (m10 - m01) / k; }
        else if (m00 > m11 && m00 > m22) { const float k = sqrtf(1.f + m00 - m11 - m22) * 2.f; q[3] = (m21 - m12) / k; q[0] = 0.25f * k; q[1] = (m01 + m10) / k; q[2] = (m02 + m20) / k; }
        else if (m11 > m22)  { const float k = sqrtf(1.f + m11 - m00 - m22) * 2.f; q[3] = (m02 - m20) / k; q[0] = (m01 + m10) / k; q[1] = 0.25f * k; q[2] = (m12 + m21) / k; }
        else                 { const float k = sqrtf(1.f + m22 - m00 - m11) * 2.f; q[3] = (m10 - m01) / k; q[0] = (m02 + m20) / k; q[1] = (m12 + m21) / k; q[2] = 0.25f * k; }
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
            Vector3 o = f.pos, X = RightOf(f), Y = f.up, Z = f.fwd;
            if (f.bSwatted && f.bOnBack)
            {
                // "лапками вверх" (show.py): переворот вокруг продольной оси за 0.45 с
                const float u = Clamp((g_fT - f.fSwatT) / 0.45f, 0.f, 1.f);
                const float a = 3.14159265f * (0.5f - 0.5f * cosf(3.14159265f * u));
                X = Norm(Add(Mul(RightOf(f), cosf(a)), Mul(f.up, sinf(a))));
                Y = Cross(Z, X);
                o = Add(o, Mul(f.up, (1.0f * u + 1.2f * sinf(3.14159265f * u)) * s));
            }
            QuatFromBasis(X, Y, Z, sc.aRot);
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
        st.bFlying   = f.bExists && f.eMode == EMode::Flight;
        st.eState    = f.bExists ? f.eState : EState::None;
        st.pszState  = pszState;
        st.pszCause  = pszCause;
        for (int i = 0; i < fly::kOutputCount; i++) st.aOut[i] = f.fFilt[i];
        for (int i = 0; i < fly::kInputCount; i++)  st.aIn[i]  = f.aIn[i];
        st.fTurn     = f.fTurn;
        st.fHunger   = f.fHunger;
        st.fDust     = f.fDust;
        st.nDrops    = (int)g_Drops.size();
        st.nEaten    = f.nEaten;
        st.nFlights  = f.nFlights;
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
    InitFacets();

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

    TickTool(eTool);
    TickPlayer(fDt);

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
        if (f.bSwatted) dt = fDt;
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
        Behave(f, s, dt, fDt);
        if (f.bSwatted && f.eMode != EMode::Fall) f.body.Update(dt, fly::SBodyCmd());
    }
    Publish(rig, opt, s);
}
