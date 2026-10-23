#pragma once

#include <atomic>
#include <stdint.h>
#include <unordered_map>
#include "renderer/xr_platform_includes.hpp"

// Состояние кнопок контроллеров. Хук не заводит своих действий OpenXR — он подсматривает
// значения, которые игра сама читает через xrGetActionState*. Какая кнопка скрывается за
// действием, определяется по его имени один раз, при xrCreateAction.
class COpenXRInputBinding
{
public:
    enum class EHand : uint8_t { Unknown, Left, Right };

    struct SController
    {
        float fTrigger    = 0.f;    // курок 0..1
        float fGrip       = 0.f;    // рукоять 0..1
        float fStickX     = 0.f;
        float fStickY     = 0.f;
        bool  bTriggerBtn = false;  // булевые "нажато" — на случай, если игра читает только их
        bool  bGripBtn    = false;
        bool  bPrimary    = false;  // X / A
        bool  bSecondary  = false;  // Y / B
        bool  bStickClick = false;
        bool  bMenu       = false;

        float TriggerValue() const { return bTriggerBtn && fTrigger < 1.f ? 1.f : fTrigger; }
        float GripValue()    const { return bGripBtn    && fGrip    < 1.f ? 1.f : fGrip;    }
    };

    SController Left;
    SController Right;

    XrPath m_LeftHandPath  = XR_NULL_PATH;
    XrPath m_RightHandPath = XR_NULL_PATH;

    // Ставится рендер-потоком: пока луч на меню, игра не видит правый курок и правый стик.
    std::atomic<bool> m_bMaskRightHand    { false };
    std::atomic<bool> m_bMaskRightTrigger { false };   // курок правой руки забрала функция мода (луч спавна)

    void OnActionCreated(XrAction action, const XrActionCreateInfo* pInfo);
    void OnBool         (const XrActionStateGetInfo* pInfo, XrActionStateBoolean*  pState);
    void OnFloat        (const XrActionStateGetInfo* pInfo, XrActionStateFloat*    pState);
    void OnVector2      (const XrActionStateGetInfo* pInfo, XrActionStateVector2f* pState);

    // Для xrCreateActionSpace: чья это поза и прицел ли это (aim), а не рукоять (grip).
    bool DescribePose(XrAction action, XrPath subactionPath, EHand& outHand, bool& outAim) const;

private:
    enum class EControl : uint8_t { None, Pose, Trigger, Grip, Primary, Secondary, StickClick, Menu, Stick, StickX, StickY };
    struct SAction { EControl eControl = EControl::None; EHand eHand = EHand::Unknown; bool bAim = false; };

    const SAction* Find    (XrAction action) const;
    EHand          Resolve (const SAction& action, XrPath subactionPath) const;
    bool           IsMasked(const SAction& action, EHand hand) const;
    SController*   Pick    (EHand hand) { return hand == EHand::Left ? &Left : hand == EHand::Right ? &Right : nullptr; }

    std::unordered_map<uint64_t, SAction> m_Actions;
};
