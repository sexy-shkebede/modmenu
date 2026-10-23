#include "xr_input.hpp"
#include <string.h>

static inline bool Has(const char* s, const char* sub) { return strstr(s, sub) != nullptr; }

void COpenXRInputBinding::OnActionCreated(XrAction action, const XrActionCreateInfo* pInfo)
{
    if (action == XR_NULL_HANDLE || !pInfo) return;

    // По спецификации OpenXR имя действия всегда строчными буквами.
    const char* n = pInfo->actionName;
    SAction a;
    a.eHand = Has(n, "left") ? EHand::Left : Has(n, "right") ? EHand::Right : EHand::Unknown;

    if (pInfo->actionType == XR_ACTION_TYPE_POSE_INPUT)
    {
        a.eControl = EControl::Pose;
        a.bAim     = Has(n, "aim") || Has(n, "pointer");
    }
    // касания, датчики приближения и силы — не нажатия, их пропускаем
    else if (!Has(n, "touch") && !Has(n, "proximity") && !Has(n, "curl") && !Has(n, "slide") && !Has(n, "force"))
    {
        if (Has(n, "thumbstick") || Has(n, "joystick") || Has(n, "trackpad"))
            a.eControl = Has(n, "click") ? EControl::StickClick
                       : Has(n, "_x")    ? EControl::StickX
                       : Has(n, "_y")    ? EControl::StickY
                       :                   EControl::Stick;
        else if (Has(n, "trigger")   || Has(n, "select"))                        a.eControl = EControl::Trigger;
        else if (Has(n, "grip")      || Has(n, "squeeze"))                       a.eControl = EControl::Grip;
        else if (Has(n, "secondary") || Has(n, "b_click") || Has(n, "y_click"))  a.eControl = EControl::Secondary;
        else if (Has(n, "primary")   || Has(n, "a_click") || Has(n, "x_click"))  a.eControl = EControl::Primary;
        else if (Has(n, "menu"))                                                 a.eControl = EControl::Menu;
    }
    m_Actions[(uint64_t)(uintptr_t)action] = a;
}

const COpenXRInputBinding::SAction* COpenXRInputBinding::Find(XrAction action) const
{
    const auto it = m_Actions.find((uint64_t)(uintptr_t)action);
    return it != m_Actions.end() ? &it->second : nullptr;
}

COpenXRInputBinding::EHand COpenXRInputBinding::Resolve(const SAction& a, XrPath subactionPath) const
{
    // Рука — из subaction path, а если его нет (действия вида left_hand_y_click) — из имени.
    if (subactionPath != XR_NULL_PATH)
        return subactionPath == m_LeftHandPath ? EHand::Left : subactionPath == m_RightHandPath ? EHand::Right : EHand::Unknown;
    return a.eHand;
}

bool COpenXRInputBinding::IsMasked(const SAction& a, EHand hand) const
{
    if (hand != EHand::Right) return false;
    if (a.eControl == EControl::Trigger && m_bMaskRightTrigger.load(std::memory_order_relaxed)) return true;
    if (!m_bMaskRightHand.load(std::memory_order_relaxed)) return false;
    return a.eControl == EControl::Trigger || a.eControl == EControl::Stick ||
           a.eControl == EControl::StickX  || a.eControl == EControl::StickY;
}

// Неактивные действия (например, профиль другого контроллера) значения не трогают —
// раньше они затирали реальные нажатия нулями.

void COpenXRInputBinding::OnBool(const XrActionStateGetInfo* pInfo, XrActionStateBoolean* pState)
{
    const SAction* a = Find(pInfo->action);
    if (!a || a->eControl == EControl::None) return;
    const EHand hand = Resolve(*a, pInfo->subactionPath);

    if (SController* c = Pick(hand); c && pState->isActive)
    {
        const bool v = pState->currentState != XR_FALSE;
        switch (a->eControl)
        {
            case EControl::Trigger:    c->bTriggerBtn = v; break;
            case EControl::Grip:       c->bGripBtn    = v; break;
            case EControl::Primary:    c->bPrimary    = v; break;
            case EControl::Secondary:  c->bSecondary  = v; break;
            case EControl::StickClick: c->bStickClick = v; break;
            case EControl::Menu:       c->bMenu       = v; break;
            default: break;
        }
    }

    if (IsMasked(*a, hand))
    {
        pState->currentState         = XR_FALSE;
        pState->changedSinceLastSync = XR_FALSE;
    }
}

void COpenXRInputBinding::OnFloat(const XrActionStateGetInfo* pInfo, XrActionStateFloat* pState)
{
    const SAction* a = Find(pInfo->action);
    if (!a || a->eControl == EControl::None) return;
    const EHand hand = Resolve(*a, pInfo->subactionPath);

    if (SController* c = Pick(hand); c && pState->isActive)
    {
        const float v = pState->currentState;
        switch (a->eControl)
        {
            case EControl::Trigger: c->fTrigger = v; break;
            case EControl::Grip:    c->fGrip    = v; break;
            case EControl::StickX:  c->fStickX  = v; break;
            case EControl::StickY:  c->fStickY  = v; break;
            default: break;
        }
    }

    if (IsMasked(*a, hand))
    {
        pState->currentState         = 0.f;
        pState->changedSinceLastSync = XR_FALSE;
    }
}

void COpenXRInputBinding::OnVector2(const XrActionStateGetInfo* pInfo, XrActionStateVector2f* pState)
{
    const SAction* a = Find(pInfo->action);
    if (!a || a->eControl != EControl::Stick) return;
    const EHand hand = Resolve(*a, pInfo->subactionPath);

    if (SController* c = Pick(hand); c && pState->isActive)
    {
        c->fStickX = pState->currentState.x;
        c->fStickY = pState->currentState.y;
    }

    if (IsMasked(*a, hand))
    {
        pState->currentState         = { 0.f, 0.f };
        pState->changedSinceLastSync = XR_FALSE;
    }
}

bool COpenXRInputBinding::DescribePose(XrAction action, XrPath subactionPath, EHand& outHand, bool& outAim) const
{
    const SAction* a = Find(action);
    if (a && a->eControl != EControl::Pose) return false;

    if (a)
    {
        outHand = Resolve(*a, subactionPath);
        outAim  = a->bAim;
    }
    else
    {
        // Действие создано до установки хука — имени не знаем, ориентируемся только по руке.
        outHand = subactionPath == XR_NULL_PATH       ? EHand::Unknown
                : subactionPath == m_LeftHandPath  ? EHand::Left
                : subactionPath == m_RightHandPath ? EHand::Right : EHand::Unknown;
        outAim  = false;
    }
    return outHand != EHand::Unknown;
}
