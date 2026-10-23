#include "minecraft.hpp"
#include "photon.hpp"
#include "unity_bridge.hpp"
#include "../config.hpp"
#include "../minecraft/blocks.hpp"
#include "../minecraft/draw.hpp"
#include "../minecraft/world.hpp"
#include "../openxr_hook/openxr_hook.hpp"

#include <math.h>
#include <mutex>
#include <time.h>
#include <unordered_map>
#include <vector>
#include <android/log.h>

namespace
{
    using minecraft::EMode;

    // ---------------------------------------------------------------- векторы (оси Unity)
    Vector3 V(float x, float y, float z)              { return Vector3(x, y, z); }
    Vector3 V(const float c[3])                       { return Vector3(c[0], c[1], c[2]); }
    Vector3 Add(const Vector3& a, const Vector3& b)   { return V(a.x + b.x, a.y + b.y, a.z + b.z); }
    Vector3 Sub(const Vector3& a, const Vector3& b)   { return V(a.x - b.x, a.y - b.y, a.z - b.z); }
    Vector3 Mul(const Vector3& a, float s)            { return V(a.x * s, a.y * s, a.z * s); }
    float   Dot(const Vector3& a, const Vector3& b)   { return a.x * b.x + a.y * b.y + a.z * b.z; }
    float   Comp(const Vector3& v, int i)             { return i == 0 ? v.x : (i == 1 ? v.y : v.z); }
    void    SetComp(Vector3& v, int i, float f)       { (i == 0 ? v.x : (i == 1 ? v.y : v.z)) = f; }
    Vector3 FaceNormal(int iFace)                     { Vector3 n = V(0.f, 0.f, 0.f); SetComp(n, iFace / 2, (iFace & 1) ? -1.f : 1.f); return n; }

    uint64_t NowNs()
    {
        timespec ts;
        clock_gettime(CLOCK_MONOTONIC, &ts);
        return (uint64_t)ts.tv_sec * 1000000000ull + (uint64_t)ts.tv_nsec;
    }

    // ---------------------------------------------------------------- состояние
    struct SBlockRt
    {
        mc::SPlaced p;
        uint32_t    hCollider    = 0;    // GC-handle невидимого коллайдера (0 — нет)
        int         iLayer       = -1;   // слой коллайдера: как у поверхности, на которую поставили
        float       fTargetAlpha = 1.f;  // 0 — за стеной
    };
    // Шаг истории для Undo: bPlaced — блоки поставили (отмена убирает), иначе убрали (отмена возвращает)
    struct SAction { bool bPlaced; std::vector<SBlockRt> blocks; };
    enum ECmd { kCmdUndo, kCmdClear };

    // Общее с меню и хуком — под g_Lock
    std::mutex            g_Lock;
    minecraft::SOptions   g_Opt;
    int                   g_iSelected = -1;
    EMode                 g_eMode     = EMode::Place;
    std::vector<int>      g_Cmds;
    minecraft::SStatus    g_Status;
    mc::SWorld            g_World;
    uint64_t              g_uCursorNs = 0;
    std::string           g_sTarget;          // какой блок сломается (для подписи)

    // Только главный поток
    std::vector<SBlockRt> g_Blocks;
    std::vector<SAction>  g_Undo;
    uint32_t              g_uNextId      = 1;
    bool                  g_bGunOn       = false;
    bool                  g_bTriggerDown = false;
    bool                  g_bModeDown    = false;
    size_t                g_iOccludeNext = 0;
    size_t                g_iAliveNext   = 0;
    float                 g_fSinceRoom   = 1.f;
    bool                  g_bPublicRoom  = false;
    Vector3               g_CamPos       = V(0.f, 0.f, 0.f);
    float                 g_fScale       = 1.f;

    constexpr size_t kMaxUndo = 256;

    void Message(const std::string& s)
    {
        std::lock_guard<std::mutex> lock(g_Lock);
        g_Status.sMessage = s;
        g_Status.iMessageId++;
    }

    bool IsTranslucent(const mc::SPlaced& p) { return (mc::Block(p.uType).uFlags & mc::kTranslucent) != 0; }

    // ---------------------------------------------------------------- соседи
    // Грань, прижатая к непрозрачному соседу того же размера, не рисуется
    uint64_t CellKey(const float c[3])
    {
        const auto Q = [](float f) { return (uint64_t)(llroundf(f * 1000.f) & 0x1FFFFF); };   // миллиметры
        return (Q(c[0]) << 42) | (Q(c[1]) << 21) | Q(c[2]);
    }

    void RebuildFaces()
    {
        std::unordered_map<uint64_t, int> cells;
        cells.reserve(g_Blocks.size() * 2);
        for (int i = 0; i < (int)g_Blocks.size(); i++) cells[CellKey(g_Blocks[i].p.c)] = i;

        for (SBlockRt& b : g_Blocks)
        {
            b.p.uHiddenFaces = 0;
            for (int f = 0; f < mc::kFaceCount; f++)
            {
                const Vector3 n = Add(V(b.p.c), Mul(FaceNormal(f), b.p.fHalf * 2.f));
                const float   c[3] = { n.x, n.y, n.z };
                const auto    it = cells.find(CellKey(c));
                if (it == cells.end()) continue;
                const mc::SPlaced& o = g_Blocks[it->second].p;
                if (fabsf(o.fHalf - b.p.fHalf) > 1e-4f || fabsf(o.c[0] - c[0]) + fabsf(o.c[1] - c[1]) + fabsf(o.c[2] - c[2]) > 3e-3f) continue;
                if (!IsTranslucent(o) || (IsTranslucent(b.p) && o.uType == b.p.uType))   // стекло к стеклу — сплошное
                    b.p.uHiddenFaces |= (uint8_t)(1u << f);
            }
        }
    }

    // ---------------------------------------------------------------- геометрия
    struct SBlockHit { int iIndex; float t; int iFace; };

    // Луч против наших блоков (метод "плит"): ближайший и грань входа
    bool RayBlocks(const Vector3& o, const Vector3& d, float fMax, SBlockHit& out)
    {
        bool bAny = false;
        out.t = fMax;
        for (int i = 0; i < (int)g_Blocks.size(); i++)
        {
            const mc::SPlaced& b = g_Blocks[i].p;
            float tMin = -1e30f, tMax = 1e30f;
            int   iAxis = 0;
            bool  bMiss = false;
            for (int a = 0; a < 3 && !bMiss; a++)
            {
                const float oa = Comp(o, a), da = Comp(d, a), lo = b.c[a] - b.fHalf, hi = b.c[a] + b.fHalf;
                if (fabsf(da) < 1e-8f) { bMiss = oa < lo || oa > hi; continue; }
                float t1 = (lo - oa) / da, t2 = (hi - oa) / da;
                if (t1 > t2) { const float t = t1; t1 = t2; t2 = t; }
                if (t1 > tMin) { tMin = t1; iAxis = a; }
                if (t2 < tMax) tMax = t2;
                bMiss = tMin > tMax;
            }
            if (bMiss || tMin < 0.f || tMin >= out.t) continue;
            out = { i, tMin, iAxis * 2 + (Comp(d, iAxis) > 0.f ? 1 : 0) };
            bAny = true;
        }
        return bAny;
    }

    // Точка на поверхности или внутри одного из наших блоков (луч Physics попал в наш коллайдер)
    bool OnOurBlock(const Vector3& p)
    {
        for (const SBlockRt& b : g_Blocks)
        {
            const float e = b.p.fHalf * 1.002f + 1e-4f;
            if (fabsf(p.x - b.p.c[0]) <= e && fabsf(p.y - b.p.c[1]) <= e && fabsf(p.z - b.p.c[2]) <= e) return true;
        }
        return false;
    }

    float SnapCenter(float f, float fCell) { return (floorf(f / fCell) + 0.5f) * fCell; }

    int FacingTowards(const Vector3& c, const Vector3& eye)
    {
        const float dx = eye.x - c.x, dz = eye.z - c.z;
        return fabsf(dx) > fabsf(dz) ? (dx > 0.f ? mc::kPosX : mc::kNegX) : (dz > 0.f ? mc::kPosZ : mc::kNegZ);
    }

    // Куда встанет новый блок
    struct SPlan
    {
        bool        bValid = false;
        Vector3     c      = V(0.f, 0.f, 0.f);
        float       fHalf  = 0.f;
        int         iAxis  = 1;
        int         iLayer = -1;
        const char* pszWhy = nullptr;   // почему нельзя
    };

    SPlan PlanOnBlock(const SBlockHit& bh, float fHalf)
    {
        const SBlockRt& b = g_Blocks[bh.iIndex];
        SPlan plan;
        plan.c      = Add(V(b.p.c), Mul(FaceNormal(bh.iFace), b.p.fHalf + fHalf));
        plan.fHalf  = fHalf;
        plan.iAxis  = bh.iFace / 2;
        plan.iLayer = b.iLayer;
        return plan;
    }

    SPlan PlanOnSurface(const ubridge::SHit& hit, float fHalf, bool bSnap)
    {
        // Нормаль — к ближайшей оси: блок встаёт вплотную к поверхности, по остальным осям — в сетку
        int iAxis = 0;
        for (int a = 1; a < 3; a++) if (fabsf(Comp(hit.normal, a)) > fabsf(Comp(hit.normal, iAxis))) iAxis = a;
        const float fSign = Comp(hit.normal, iAxis) >= 0.f ? 1.f : -1.f;
        SPlan plan;
        plan.fHalf  = fHalf;
        plan.iAxis  = iAxis;
        plan.iLayer = hit.iLayer;
        for (int a = 0; a < 3; a++)
        {
            const float p = Comp(hit.point, a);
            SetComp(plan.c, a, a == iAxis ? p + fSign * fHalf : (bSnap ? SnapCenter(p, fHalf * 2.f) : p));
        }
        return plan;
    }

    SPlan PlanInAir(const Vector3& at, float fHalf, bool bSnap)
    {
        SPlan plan;
        plan.fHalf = fHalf;
        for (int a = 0; a < 3; a++) SetComp(plan.c, a, bSnap ? SnapCenter(Comp(at, a), fHalf * 2.f) : Comp(at, a));
        return plan;
    }

    bool Overlaps(const float c[3], float fHalf)
    {
        for (const SBlockRt& b : g_Blocks)
        {
            const float e = (b.p.fHalf + fHalf) * 0.98f;
            if (fabsf(c[0] - b.p.c[0]) < e && fabsf(c[1] - b.p.c[1]) < e && fabsf(c[2] - b.p.c[2]) < e) return true;
        }
        return false;
    }

    void Validate(SPlan& plan)
    {
        plan.bValid = false;
        if ((int)g_Blocks.size() >= cfg::kMaxBlocks) { plan.pszWhy = "Block limit reached"; return; }
        const float c[3] = { plan.c.x, plan.c.y, plan.c.z };
        if (Overlaps(c, plan.fHalf)) { plan.pszWhy = "There's already a block there"; return; }
        // Не замуровать себе голову
        const float e = plan.fHalf + 0.12f * g_fScale;
        if (fabsf(plan.c.x - g_CamPos.x) < e && fabsf(plan.c.y - g_CamPos.y) < e && fabsf(plan.c.z - g_CamPos.z) < e)
        {
            plan.pszWhy = "Too close to your head";
            return;
        }
        plan.bValid = true;
    }

    // ---------------------------------------------------------------- коллайдеры
    bool SolidWanted(const minecraft::SOptions& opt) { return opt.bSolid && !g_bPublicRoom && ubridge::CanCreateColliders(); }

    void DropCollider(SBlockRt& b)
    {
        if (b.hCollider) ubridge::DestroyObject(b.hCollider);
        b.hCollider = 0;
    }

    void SyncColliders(bool bWanted)
    {
        // Создаём понемногу за тик (Undo большой постройки не даст рывка), проверяем,
        // не уничтожила ли их игра (например, при смене сцены)
        int nBudget = 16;
        for (SBlockRt& b : g_Blocks)
        {
            if (!bWanted) { DropCollider(b); continue; }
            if (!b.hCollider && nBudget-- > 0)
                b.hCollider = ubridge::CreateBoxCollider(V(b.p.c), b.p.fHalf * 2.f, b.iLayer);
        }
        for (int i = 0; i < 4 && !g_Blocks.empty(); i++)
        {
            SBlockRt& b = g_Blocks[g_iAliveNext++ % g_Blocks.size()];
            if (b.hCollider && !ubridge::IsAlive(b.hCollider)) DropCollider(b);
        }
    }

    // ---------------------------------------------------------------- действия
    void PushUndo(bool bPlaced, std::vector<SBlockRt> blocks)
    {
        for (SBlockRt& b : blocks) b.hCollider = 0;
        g_Undo.push_back({ bPlaced, std::move(blocks) });
        if (g_Undo.size() > kMaxUndo) g_Undo.erase(g_Undo.begin());
    }

    void PlaceBlock(const SPlan& plan, int iType)
    {
        SBlockRt b;
        b.p.uId          = g_uNextId++;
        b.p.c[0]         = plan.c.x; b.p.c[1] = plan.c.y; b.p.c[2] = plan.c.z;
        b.p.fHalf        = plan.fHalf;
        b.p.uType        = (uint16_t)iType;
        b.p.uAxis        = (uint8_t)((mc::Block(iType).uFlags & mc::kPillar) ? plan.iAxis : 1);
        b.p.uFacing      = (uint8_t)FacingTowards(plan.c, g_CamPos);
        b.p.uHiddenFaces = 0;
        b.p.fAlpha       = 1.f;
        b.p.fScale       = 0.f;
        b.iLayer         = plan.iLayer;
        g_Blocks.push_back(b);
        PushUndo(true, { b });
        RebuildFaces();
    }

    void BreakBlock(int iIndex)
    {
        SBlockRt b = g_Blocks[iIndex];
        DropCollider(g_Blocks[iIndex]);
        g_Blocks.erase(g_Blocks.begin() + iIndex);
        PushUndo(false, { b });
        RebuildFaces();
    }

    void DoUndo()
    {
        if (g_Undo.empty()) { Message("Nothing to undo"); return; }
        SAction a = std::move(g_Undo.back());
        g_Undo.pop_back();
        if (a.bPlaced)
        {
            for (const SBlockRt& u : a.blocks)
                for (size_t i = 0; i < g_Blocks.size(); i++)
                    if (g_Blocks[i].p.uId == u.p.uId)
                    {
                        DropCollider(g_Blocks[i]);
                        g_Blocks.erase(g_Blocks.begin() + (long)i);
                        break;
                    }
        }
        else
            for (SBlockRt& u : a.blocks)
            {
                if ((int)g_Blocks.size() >= cfg::kMaxBlocks) break;
                if (Overlaps(u.p.c, u.p.fHalf)) continue;   // клетку уже заняли новым блоком
                u.p.fScale = 0.f;
                g_Blocks.push_back(u);   // коллайдер вернёт SyncColliders
            }
        RebuildFaces();
        Message(a.bPlaced ? "Undone: block removed" : "Undone: " + std::to_string(a.blocks.size()) + (a.blocks.size() == 1 ? " block is back" : " blocks are back"));
    }

    void DoClear()
    {
        if (g_Blocks.empty()) { Message("No blocks to clear"); return; }
        const size_t n = g_Blocks.size();
        for (SBlockRt& b : g_Blocks) DropCollider(b);
        PushUndo(false, g_Blocks);
        g_Blocks.clear();
        Message("Cleared " + std::to_string(n) + (n == 1 ? " block. Undo brings it back" : " blocks. Undo brings them back"));
    }

    // ---------------------------------------------------------------- за стенами
    // Луч от блока к глазу: упёрся в стену игры — блок прячется. Пробуем центр и грань,
    // смотрящую на глаз; хватит одного свободного. За тик — часть блоков, по кругу.
    void TickOcclusion(bool bEnabled, float fDt)
    {
        const size_t n = g_Blocks.size();
        if (!bEnabled)
            for (SBlockRt& b : g_Blocks) b.fTargetAlpha = 1.f;
        else if (n)
        {
            const size_t nBudget = n / 6 < 16 ? 16 : (n / 6 > 96 ? 96 : n / 6);
            for (size_t k = 0; k < nBudget && k < n; k++)
            {
                SBlockRt&     b = g_Blocks[g_iOccludeNext++ % n];
                const Vector3 c = V(b.p.c);
                const Vector3 toEye = Sub(g_CamPos, c);
                int iAxis = 0;
                for (int a = 1; a < 3; a++) if (fabsf(Comp(toEye, a)) > fabsf(Comp(toEye, iAxis))) iAxis = a;
                Vector3 face = c;
                SetComp(face, iAxis, Comp(c, iAxis) + (Comp(toEye, iAxis) > 0.f ? 0.98f : -0.98f) * b.p.fHalf);

                bool bVisible = false;
                for (const Vector3& from : { c, face })
                {
                    const Vector3 d    = Sub(g_CamPos, from);
                    const float   fLen = sqrtf(Dot(d, d));
                    const float   fMax = fLen - 0.3f * g_fScale;   // до головы и рук не достаём
                    ubridge::SHit hit {};
                    if (fMax <= 0.f || !ubridge::Raycast(from, Mul(d, 1.f / fLen), fMax, hit) || OnOurBlock(hit.point))
                    {
                        bVisible = true;
                        break;
                    }
                }
                b.fTargetAlpha = bVisible ? 1.f : 0.f;
            }
        }
        const float k = 1.f - expf(-12.f * fDt);
        for (SBlockRt& b : g_Blocks)
        {
            b.p.fAlpha += (b.fTargetAlpha - b.p.fAlpha) * k;
            b.p.fScale  = b.p.fScale + fDt / 0.12f > 1.f ? 1.f : b.p.fScale + fDt / 0.12f;
        }
    }

    // ---------------------------------------------------------------- луч
    void GunOff()
    {
        if (!g_bGunOn) return;
        g_bGunOn       = false;
        g_bTriggerDown = false;
        if (g_pOpenXRHook)
        {
            g_pOpenXRHook->SetSpawnBeam(false, -1.f);
            g_pOpenXRHook->m_Input.m_bMaskRightTrigger.store(false, std::memory_order_relaxed);
        }
    }

    bool ModeButton(const COpenXRInputBinding::SController& r)
    {
        return cfg::kModeButton == cfg::EModeButton::A ? r.bPrimary
             : cfg::kModeButton == cfg::EModeButton::B ? r.bSecondary : false;
    }

    // Луч правой руки: что под ним (наш блок, стена игры или воздух) и что сделает курок
    mc::SCursor TickGun(int iSelected, EMode eMode, const minecraft::SOptions& opt, std::string& outTarget)
    {
        mc::SCursor  cursor;
        COpenXRHook* pHook = g_pOpenXRHook;
        if (iSelected < 0 || !pHook) { GunOff(); return cursor; }

        // Кнопка режима — по фронту нажатия
        const bool bMode = ModeButton(pHook->m_Input.Right);
        if (bMode && !g_bModeDown)
        {
            eMode = eMode == EMode::Place ? EMode::Break : EMode::Place;
            {
                std::lock_guard<std::mutex> lock(g_Lock);
                g_eMode = eMode;
            }
            Message(eMode == EMode::Break ? "Break mode: the trigger removes blocks" : "Place mode: the trigger places blocks");
        }
        g_bModeDown = bMode;

        XrVector3f dirHead, posHead;
        Vector3    hand, dir;
        if (!pHook->GetRightAimInHead(dirHead, &posHead) ||
            !ubridge::HeadToWorld(V(posHead.x, posHead.y, -posHead.z), V(dirHead.x, dirHead.y, -dirHead.z), hand, dir))
        {
            pHook->SetSpawnBeam(false, -1.f);
            return cursor;
        }

        // Луч стартует в 5 см перед рукой — коллайдеры своих рук не мешают
        const float   s      = g_fScale;
        const Vector3 origin = Add(hand, Mul(dir, 0.05f * s));
        const float   fMax   = cfg::kSpawnBeamMaxM * s;
        SBlockHit     bh {};
        ubridge::SHit wh {};
        bool bBlock = RayBlocks(origin, dir, fMax, bh);
        bool bWorld = ubridge::Raycast(origin, dir, fMax, wh, eMode == EMode::Place);
        if (bWorld && (OnOurBlock(wh.point) || (bBlock && bh.t <= wh.fDistance + 0.01f * s))) bWorld = false;   // это наш коллайдер
        if (bWorld && bBlock && wh.fDistance < bh.t) bBlock = false;

        float fBeam = -1.f;   // длина луча до точки попадания, м
        int   iBreak = -1;
        SPlan plan;
        const float fHalf = opt.fSize * 0.5f * s;
        if (eMode == EMode::Break)
        {
            if (bBlock)
            {
                iBreak = bh.iIndex;
                const mc::SPlaced& b = g_Blocks[iBreak].p;
                cursor.bShow = true;
                cursor.c[0]  = b.c[0]; cursor.c[1] = b.c[1]; cursor.c[2] = b.c[2];
                cursor.fHalf = b.fHalf;
                outTarget    = mc::Block(b.uType).pszName;
                fBeam        = bh.t / s;
            }
            else if (bWorld) fBeam = wh.fDistance / s;
        }
        else
        {
            if (bBlock)      { plan = PlanOnBlock(bh, fHalf);                   fBeam = bh.t / s; }
            else if (bWorld) { plan = PlanOnSurface(wh, fHalf, opt.bSnap);       fBeam = wh.fDistance / s; }
            else             { plan = PlanInAir(Add(origin, Mul(dir, cfg::kAirPlaceM * s)), fHalf, opt.bSnap); fBeam = cfg::kAirPlaceM; }
            Validate(plan);
            if (plan.bValid)
            {
                cursor.bShow   = true;
                cursor.bGhost  = true;
                cursor.c[0]    = plan.c.x; cursor.c[1] = plan.c.y; cursor.c[2] = plan.c.z;
                cursor.fHalf   = plan.fHalf;
                cursor.uType   = (uint16_t)iSelected;
                cursor.uAxis   = (uint8_t)((mc::Block(iSelected).uFlags & mc::kPillar) ? plan.iAxis : 1);
                cursor.uFacing = (uint8_t)FacingTowards(plan.c, g_CamPos);
            }
        }
        pHook->SetSpawnBeam(true, fBeam);
        g_bGunOn = true;

        // Курок забираем у игры, пока в руке блок и луч не на меню
        const bool  bOnMenu  = pHook->IsPointerOnMenu();
        pHook->m_Input.m_bMaskRightTrigger.store(!bOnMenu, std::memory_order_relaxed);
        const float fTrigger = pHook->m_Input.Right.TriggerValue();
        const bool  bDown    = g_bTriggerDown ? fTrigger > cfg::kTriggerRelease : fTrigger >= cfg::kTriggerPress;
        if (bDown && !g_bTriggerDown && !bOnMenu)
        {
            if (eMode == EMode::Break)
            {
                if (iBreak >= 0) BreakBlock(iBreak);
                else             Message("Point the beam at one of your blocks");
            }
            else if (plan.bValid) PlaceBlock(plan, iSelected);
            else if (plan.pszWhy) Message(plan.pszWhy);
        }
        g_bTriggerDown = bDown;
        return cursor;
    }

    void Publish(const mc::SCursor& cursor, const mc::SRig& rig, const minecraft::SOptions& opt, const std::string& sTarget, bool bSolidSupported)
    {
        std::lock_guard<std::mutex> lock(g_Lock);
        g_World.blocks.resize(g_Blocks.size());
        for (size_t i = 0; i < g_Blocks.size(); i++) g_World.blocks[i] = g_Blocks[i].p;
        g_World.rig         = rig;
        g_World.cursor      = cursor;
        g_World.fBrightness = opt.fBrightness / 100.f;
        g_uCursorNs         = NowNs();
        g_sTarget           = sTarget;
        g_Status.nBlocks         = (int)g_Blocks.size();
        g_Status.bCanUndo        = !g_Undo.empty();
        g_Status.bSolidSupported = bSolidSupported;
        g_Status.bPublicRoom     = g_bPublicRoom;
    }
}

// ---------------------------------------------------------------- API для меню
minecraft::SStatus minecraft::GetStatus()
{
    std::lock_guard<std::mutex> lock(g_Lock);
    SStatus s   = g_Status;
    s.iSelected = g_iSelected;
    s.eMode     = g_eMode;
    return s;
}

minecraft::SOptions minecraft::GetOptions()                 { std::lock_guard<std::mutex> l(g_Lock); return g_Opt; }
void                minecraft::SetOptions(const SOptions& o) { std::lock_guard<std::mutex> l(g_Lock); g_Opt = o; }
void                minecraft::SetMode(EMode e)             { std::lock_guard<std::mutex> l(g_Lock); g_eMode = e; }
void                minecraft::Undo()                       { std::lock_guard<std::mutex> l(g_Lock); g_Cmds.push_back(kCmdUndo); }
void                minecraft::ClearAll()                   { std::lock_guard<std::mutex> l(g_Lock); g_Cmds.push_back(kCmdClear); }

void minecraft::Select(int iType)
{
    std::lock_guard<std::mutex> lock(g_Lock);
    g_iSelected = (iType >= 0 && iType < mc::BlockCount()) ? iType : -1;
    g_Status.sMessage = g_iSelected < 0 ? "Hand is empty: the beam is off"
                                        : std::string(mc::Block(g_iSelected).pszName) + (g_eMode == EMode::Place ? ": pull the right trigger to place" : ": break mode is on");
    g_Status.iMessageId++;
}

std::string minecraft::BeamLabel()
{
    std::lock_guard<std::mutex> lock(g_Lock);
    if (g_iSelected < 0) return std::string();
    if (g_eMode == EMode::Break) return g_sTarget.empty() ? std::string("Break") : "Break " + g_sTarget;
    return mc::Block(g_iSelected).pszName;
}

// ---------------------------------------------------------------- хук
bool minecraft::WorldActive()
{
    std::lock_guard<std::mutex> lock(g_Lock);
    const bool bCursor = g_World.cursor.bShow && NowNs() - g_uCursorNs < 250000000ull;   // главный поток жив
    return g_World.rig.bValid && (!g_World.blocks.empty() || bCursor);
}

void minecraft::DrawWorld(const SWorldEye* aEyes, int nEyes)
{
    static mc::SWorld s_World;   // копия для рендер-потока: рисуем без блокировки
    {
        std::lock_guard<std::mutex> lock(g_Lock);
        s_World.blocks      = g_World.blocks;
        s_World.rig         = g_World.rig;
        s_World.cursor      = g_World.cursor;
        s_World.fBrightness = g_World.fBrightness;
        if (NowNs() - g_uCursorNs >= 250000000ull) s_World.cursor.bShow = false;
    }
    mc::DrawWorld(ImGui::GetBackgroundDrawList(), mc::ContextAtlas(), s_World, aEyes, nEyes);
}

// ---------------------------------------------------------------- главный поток
void minecraft::Tick(float fDt)
{
    int              iSelected;
    EMode            eMode;
    SOptions         opt;
    std::vector<int> cmds;
    {
        std::lock_guard<std::mutex> lock(g_Lock);
        iSelected = g_iSelected;
        eMode     = g_eMode;
        opt       = g_Opt;
        cmds.swap(g_Cmds);
    }
    for (int c : cmds) (c == kCmdUndo ? DoUndo : DoClear)();

    // Ни блоков, ни блока в руке — игру не трогаем
    mc::SRig rig;
    if (g_Blocks.empty() && iSelected < 0)
    {
        GunOff();
        Publish(mc::SCursor(), rig, opt, std::string(), true);
        return;
    }

    Vector3    pos, cam;
    Quaternion rot;
    float      fScale = 1.f;
    if (ubridge::GetRig(pos, rot, fScale, cam))
    {
        rig.bValid = true;
        rig.aPos[0] = pos.x; rig.aPos[1] = pos.y; rig.aPos[2] = pos.z;
        rig.aRot[0] = rot.x; rig.aRot[1] = rot.y; rig.aRot[2] = rot.z; rig.aRot[3] = rot.w;
        rig.fScale  = fScale;
        g_CamPos    = cam;
        g_fScale    = fScale;
    }

    // Комната Photon — дважды в секунду: в публичной коллайдеры выключаются
    g_fSinceRoom += fDt;
    if (g_fSinceRoom >= 0.5f)
    {
        g_fSinceRoom = 0.f;
        const bool bWasPublic = g_bPublicRoom;
        g_bPublicRoom = photon::GetRoom() == photon::ERoom::Public;
        if (g_bPublicRoom && !bWasPublic && opt.bSolid && !g_Blocks.empty())
            Message("Public room: blocks are no longer solid");
    }

    std::string sTarget;
    const mc::SCursor cursor = rig.bValid ? TickGun(iSelected, eMode, opt, sTarget) : mc::SCursor();
    if (!rig.bValid) GunOff();
    SyncColliders(SolidWanted(opt));
    if (rig.bValid) TickOcclusion(opt.bHideBehind, fDt);
    Publish(cursor, rig, opt, sTarget, ubridge::CanCreateColliders());
}
