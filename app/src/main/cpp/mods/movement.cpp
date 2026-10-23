#include "mods.hpp"
#include "photon.hpp"
#include "unity_bridge.hpp"
#include "../config.hpp"
#include "../openxr_hook/openxr_hook.hpp"

// Movement: Fly, Noclip, Platforms. Всё выполняется в главном потоке игры (mods::Tick).
namespace mods
{
    namespace
    {
        bool FlyButtonHeld()
        {
            const COpenXRInputBinding::SController& R = g_pOpenXRHook->m_Input.Right;
            switch (cfg::kFlyButton)
            {
                case cfg::EFlyButton::A:       return R.bPrimary;
                case cfg::EFlyButton::B:       return R.bSecondary;
                case cfg::EFlyButton::Grip:    return R.GripValue()    > 0.5f;
                case cfg::EFlyButton::Trigger: return R.TriggerValue() > 0.5f;
            }
            return false;
        }

        // OpenXR — правая система (-Z вперёд), Unity — левая (+Z вперёд): меняем знак Z
        Vector3 ToUnity(const XrVector3f& v) { return Vector3(v.x, v.y, -v.z); }

        // Положение ладони (grip) в мире игры
        bool HandWorld(bool bRight, Vector3& out)
        {
            XrVector3f dir, pos;
            Vector3    dirWorld;
            return g_pOpenXRHook->GetHandInHead(bRight, false, dir, &pos) &&
                   ubridge::HeadToWorld(ToUnity(pos), ToUnity(dir), out, dirWorld);
        }

        bool g_bNoclipOn = false;

        struct SGrab
        {
            bool    bDown = false;   // курок нажат (с гистерезисом)
            bool    bHeld = false;   // рука держится за воздух
            Vector3 anchor;          // где схватились, в мире игры
        };
        SGrab g_aGrab[2];            // 0 — левая, 1 — правая
    }

    void TickFly(float fDt)
    {
        if (!Fly.bEnabled || !g_pOpenXRHook || fDt <= 0.f || !FlyButtonHeld()) return;
        XrVector3f dir;
        if (!g_pOpenXRHook->GetRightAimInHead(dir)) return;
        ubridge::MoveLocalPlayer(ToUnity(dir), Fly.fSpeed * fDt);
    }

    void TickNoclip()
    {
        // Держишь кнопку — столкновения игрока выключены (подтверждаем каждый тик: игра
        // может включить их обратно); отпустил — возвращаем один раз.
        const bool bWant = Noclip.bEnabled && g_pOpenXRHook && FlyButtonHeld();
        if (bWant)
        {
            ubridge::SetPlayerCollisions(false);
            g_bNoclipOn = true;
        }
        else if (g_bNoclipOn)
        {
            ubridge::SetPlayerCollisions(true);
            g_bNoclipOn = false;
        }
    }

    void TickPlatforms()
    {
        COpenXRHook* pHook = g_pOpenXRHook;
        if (!pHook) return;
        if (!Platforms.bEnabled)
        {
            if (g_aGrab[0].bDown || g_aGrab[1].bDown)
            {
                g_aGrab[0] = g_aGrab[1] = SGrab();
                pHook->SetGrabBalls(false, false);
            }
            return;
        }

        // Правый курок занят, пока луч руки на меню или включён луч спавна
        const bool bRightBusy = pHook->IsPointerOnMenu() || !photon::GetArmedItem().sName.empty();

        Vector3 sum(0.f, 0.f, 0.f);
        int     nHeld = 0;
        for (int i = 0; i < 2; i++)
        {
            SGrab&      g        = g_aGrab[i];
            const float fTrigger = (i ? pHook->m_Input.Right : pHook->m_Input.Left).TriggerValue();
            const bool  bDown    = g.bDown ? fTrigger > cfg::kTriggerRelease : fTrigger >= cfg::kTriggerPress;
            Vector3     hand;
            const bool  bHand    = HandWorld(i == 1, hand);

            if (bDown && !g.bDown && bHand && !(i == 1 && bRightBusy))
            {
                g.bHeld  = true;       // схватились за воздух здесь
                g.anchor = hand;
            }
            if (!bDown) g.bHeld = false;
            g.bDown = bDown;

            if (g.bHeld && bHand)
            {
                sum = Vector3(sum.x + g.anchor.x - hand.x, sum.y + g.anchor.y - hand.y, sum.z + g.anchor.z - hand.z);
                nHeld++;
            }
        }

        // Тело сдвигается так, чтобы схваченная рука осталась на месте (с двумя руками — в среднем).
        // Скорость гасится и при нулевом сдвиге — поэтому на платформе игрок висит.
        if (nHeld)
            ubridge::MovePlayerBy(Vector3(sum.x / (float)nHeld, sum.y / (float)nHeld, sum.z / (float)nHeld));
        pHook->SetGrabBalls(g_aGrab[0].bHeld, g_aGrab[1].bHeld);
    }
}
