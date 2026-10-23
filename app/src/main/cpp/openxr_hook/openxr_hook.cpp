#include "openxr_hook.hpp"
#include "xr_math.hpp"
#include "../config.hpp"

#include <dlfcn.h>
#include <float.h>
#include <math.h>
#include <string.h>
#include <time.h>
#include <sys/prctl.h>
#include <android/log.h>

#include "dobby.h"
#include "imgui.h"

COpenXRHook* g_pOpenXRHook = nullptr;

// ============================================================================
//  Оригиналы перехваченных функций
// ============================================================================
static PFN_xrGetInstanceProcAddr    s_pfnGetInstanceProcAddr    = nullptr;   // трамплин Dobby
static PFN_xrCreateSession          s_pfnCreateSession          = nullptr;
static PFN_xrCreateAction           s_pfnCreateAction           = nullptr;
static PFN_xrCreateActionSpace      s_pfnCreateActionSpace      = nullptr;
static PFN_xrGetActionStateBoolean  s_pfnGetActionStateBoolean  = nullptr;
static PFN_xrGetActionStateFloat    s_pfnGetActionStateFloat    = nullptr;
static PFN_xrGetActionStateVector2f s_pfnGetActionStateVector2f = nullptr;
static PFN_xrEndFrame               s_pfnEndFrame               = nullptr;
static PFN_xrSyncActions            s_pfnSyncActions            = nullptr;

// ============================================================================
//  Перехватчики: сначала оригинал, потом наша обработка результата
// ============================================================================
static XRAPI_ATTR XrResult XRAPI_CALL Hook_xrGetInstanceProcAddr(XrInstance instance, const char* pszName, PFN_xrVoidFunction* pFunc)
{
    const XrResult res = s_pfnGetInstanceProcAddr(instance, pszName, pFunc);
    if (res == XR_SUCCESS && pszName && pFunc && *pFunc && g_pOpenXRHook)
        g_pOpenXRHook->OnGetInstanceProcAddr(pszName, pFunc);
    return res;
}

static XRAPI_ATTR XrResult XRAPI_CALL Hook_xrCreateSession(XrInstance instance, const XrSessionCreateInfo* pInfo, XrSession* pSession)
{
    const XrResult res = s_pfnCreateSession(instance, pInfo, pSession);
    if (XR_SUCCEEDED(res) && pInfo && pSession && g_pOpenXRHook)
        g_pOpenXRHook->OnCreateSession(instance, pInfo, *pSession);
    return res;
}

static XRAPI_ATTR XrResult XRAPI_CALL Hook_xrCreateAction(XrActionSet actionSet, const XrActionCreateInfo* pInfo, XrAction* pAction)
{
    const XrResult res = s_pfnCreateAction(actionSet, pInfo, pAction);
    if (XR_SUCCEEDED(res) && pInfo && pAction && g_pOpenXRHook)
        g_pOpenXRHook->m_Input.OnActionCreated(*pAction, pInfo);
    return res;
}

static XRAPI_ATTR XrResult XRAPI_CALL Hook_xrCreateActionSpace(XrSession session, const XrActionSpaceCreateInfo* pInfo, XrSpace* pSpace)
{
    const XrResult res = s_pfnCreateActionSpace(session, pInfo, pSpace);
    if (XR_SUCCEEDED(res) && pInfo && pSpace && g_pOpenXRHook)
        g_pOpenXRHook->OnCreateActionSpace(pInfo, *pSpace);
    return res;
}

static XRAPI_ATTR XrResult XRAPI_CALL Hook_xrGetActionStateBoolean(XrSession session, const XrActionStateGetInfo* pInfo, XrActionStateBoolean* pState)
{
    const XrResult res = s_pfnGetActionStateBoolean(session, pInfo, pState);
    if (XR_SUCCEEDED(res) && pInfo && pState && g_pOpenXRHook)
        g_pOpenXRHook->m_Input.OnBool(pInfo, pState);
    return res;
}

static XRAPI_ATTR XrResult XRAPI_CALL Hook_xrGetActionStateFloat(XrSession session, const XrActionStateGetInfo* pInfo, XrActionStateFloat* pState)
{
    const XrResult res = s_pfnGetActionStateFloat(session, pInfo, pState);
    if (XR_SUCCEEDED(res) && pInfo && pState && g_pOpenXRHook)
        g_pOpenXRHook->m_Input.OnFloat(pInfo, pState);
    return res;
}

static XRAPI_ATTR XrResult XRAPI_CALL Hook_xrGetActionStateVector2f(XrSession session, const XrActionStateGetInfo* pInfo, XrActionStateVector2f* pState)
{
    const XrResult res = s_pfnGetActionStateVector2f(session, pInfo, pState);
    if (XR_SUCCEEDED(res) && pInfo && pState && g_pOpenXRHook)
        g_pOpenXRHook->m_Input.OnVector2(pInfo, pState);
    return res;
}

static XRAPI_ATTR XrResult XRAPI_CALL Hook_xrEndFrame(XrSession session, const XrFrameEndInfo* pInfo)
{
    const XrFrameEndInfo* pSubmit = pInfo;
    if (pInfo && g_pOpenXRHook)
        pSubmit = g_pOpenXRHook->OnEndFrame(session, pInfo);
    return s_pfnEndFrame(session, pSubmit);
}

// Unity синхронизирует ввод OpenXR из главного потока — отсюда функции мода могут трогать игру.
static XRAPI_ATTR XrResult XRAPI_CALL Hook_xrSyncActions(XrSession session, const XrActionsSyncInfo* pInfo)
{
    const XrResult res = s_pfnSyncActions(session, pInfo);
    if (XR_SUCCEEDED(res) && g_pOpenXRHook)
        g_pOpenXRHook->OnSyncActions();
    return res;
}

// Какие функции подменяем, когда игра запрашивает их через xrGetInstanceProcAddr.
struct SProxy { const char* pszName; PFN_xrVoidFunction* ppOriginal; PFN_xrVoidFunction pfnHook; };

#define XR_PROXY(Fn) { "xr" #Fn, reinterpret_cast<PFN_xrVoidFunction*>(&s_pfn##Fn), reinterpret_cast<PFN_xrVoidFunction>(&Hook_xr##Fn) }
static const SProxy s_aProxies[] =
{
    XR_PROXY(CreateSession),
    XR_PROXY(CreateAction),
    XR_PROXY(CreateActionSpace),
    XR_PROXY(GetActionStateBoolean),
    XR_PROXY(GetActionStateFloat),
    XR_PROXY(GetActionStateVector2f),
    XR_PROXY(EndFrame),
    XR_PROXY(SyncActions),
};
#undef XR_PROXY

template <typename TFn>
static TFn ResolveXr(XrInstance instance, const char* pszName)
{
    PFN_xrVoidFunction pfn = nullptr;
    if (!s_pfnGetInstanceProcAddr || s_pfnGetInstanceProcAddr(instance, pszName, &pfn) != XR_SUCCESS) return nullptr;
    return reinterpret_cast<TFn>(pfn);
}

static uint64_t NowNs()
{
    timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ull + (uint64_t)ts.tv_nsec;
}

// ============================================================================
bool COpenXRHook::Initialize(void* hOpenXRLoader)
{
    g_pOpenXRHook = this;
    void* pTarget = hOpenXRLoader ? dlsym(hOpenXRLoader, "xrGetInstanceProcAddr") : nullptr;
    if (!pTarget) return false;
    return DobbyHook(pTarget, reinterpret_cast<dobby_dummy_func_t>(&Hook_xrGetInstanceProcAddr),
                     reinterpret_cast<dobby_dummy_func_t*>(&s_pfnGetInstanceProcAddr)) == 0;
}

void COpenXRHook::OnGetInstanceProcAddr(const char* pszName, PFN_xrVoidFunction* pFunc)
{
    for (const SProxy& proxy : s_aProxies)
    {
        if (strcmp(pszName, proxy.pszName) != 0) continue;
        // Защита от зацикливания: наш же перехватчик оригиналом не считаем.
        if (*pFunc != proxy.pfnHook) *proxy.ppOriginal = *pFunc;
        *pFunc = proxy.pfnHook;
        return;
    }
}

void COpenXRHook::OnCreateSession(XrInstance instance, const XrSessionCreateInfo* pInfo, XrSession session)
{
    // Всё, что хук вызывает сам, берём напрямую у загрузчика — не зависим от того,
    // запрашивала ли эти функции сама игра.
    m_pfnStringToPath         = ResolveXr<PFN_xrStringToPath>        (instance, "xrStringToPath");
    m_pfnCreateReferenceSpace = ResolveXr<PFN_xrCreateReferenceSpace>(instance, "xrCreateReferenceSpace");
    m_pfnLocateSpace          = ResolveXr<PFN_xrLocateSpace>         (instance, "xrLocateSpace");

    SXrSwapchainApi api;
    api.pfnEnumerateFormats = ResolveXr<PFN_xrEnumerateSwapchainFormats>(instance, "xrEnumerateSwapchainFormats");
    api.pfnCreate           = ResolveXr<PFN_xrCreateSwapchain>          (instance, "xrCreateSwapchain");
    api.pfnDestroy          = ResolveXr<PFN_xrDestroySwapchain>         (instance, "xrDestroySwapchain");
    api.pfnEnumerateImages  = ResolveXr<PFN_xrEnumerateSwapchainImages> (instance, "xrEnumerateSwapchainImages");
    api.pfnAcquire          = ResolveXr<PFN_xrAcquireSwapchainImage>    (instance, "xrAcquireSwapchainImage");
    api.pfnWait             = ResolveXr<PFN_xrWaitSwapchainImage>       (instance, "xrWaitSwapchainImage");
    api.pfnRelease          = ResolveXr<PFN_xrReleaseSwapchainImage>    (instance, "xrReleaseSwapchainImage");

    {
        std::lock_guard<std::mutex> lock(m_SpacesLock);
        m_aLeftSpaces.clear();    // пространства прошлой сессии умерли вместе с ней
        m_aRightSpaces.clear();
    }
    m_Session      = session;
    m_WorldSpace   = XR_NULL_HANDLE;
    m_ViewSpace    = XR_NULL_HANDLE;
    m_bHavePose    = false;
    m_bTriggerDown = false;
    m_bPressOnMenu = false;
    m_bPointerFree = true;
    m_Input.m_bMaskRightHand.store(false, std::memory_order_relaxed);
    m_bPointerOnMenu.store(false, std::memory_order_relaxed);

    if (m_pfnStringToPath)
    {
        m_pfnStringToPath(instance, "/user/hand/left",  &m_Input.m_LeftHandPath);
        m_pfnStringToPath(instance, "/user/hand/right", &m_Input.m_RightHandPath);
    }

    // Своё пространство LOCAL: и позы рук, и слой меню считаются в нём, поэтому
    // результат не зависит от того, в каких пространствах работает сама игра.
    if (m_pfnCreateReferenceSpace)
    {
        XrReferenceSpaceCreateInfo spaceInfo = { XR_TYPE_REFERENCE_SPACE_CREATE_INFO };
        spaceInfo.referenceSpaceType               = XR_REFERENCE_SPACE_TYPE_LOCAL;
        spaceInfo.poseInReferenceSpace.orientation.w = 1.f;
        if (m_pfnCreateReferenceSpace(session, &spaceInfo, &m_WorldSpace) != XR_SUCCESS)
            m_WorldSpace = XR_NULL_HANDLE;

        // VIEW — поза головы: по ней направление руки переводится в оси камеры игры.
        spaceInfo.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_VIEW;
        if (m_pfnCreateReferenceSpace(session, &spaceInfo, &m_ViewSpace) != XR_SUCCESS)
            m_ViewSpace = XR_NULL_HANDLE;
    }

    const XrBaseInStructure* pBinding = nullptr;
    for (auto* p = static_cast<const XrBaseInStructure*>(pInfo->next); p; p = p->next)
        if (p->type == XR_TYPE_GRAPHICS_BINDING_VULKAN_KHR || p->type == XR_TYPE_GRAPHICS_BINDING_OPENGL_ES_ANDROID_KHR)
        {
            pBinding = p;
            break;
        }

    m_Renderer.Bind(session, pBinding, api, cfg::kPanelResX, cfg::kPanelResY + cfg::kSpriteStripH);
}

void COpenXRHook::OnCreateActionSpace(const XrActionSpaceCreateInfo* pInfo, XrSpace space)
{
    COpenXRInputBinding::EHand eHand = COpenXRInputBinding::EHand::Unknown;
    bool bAim = false;
    if (!m_Input.DescribePose(pInfo->action, pInfo->subactionPath, eHand, bAim)) return;

    std::lock_guard<std::mutex> lock(m_SpacesLock);
    if (eHand == COpenXRInputBinding::EHand::Left)  m_aLeftSpaces.push_back({ space, bAim });
    if (eHand == COpenXRInputBinding::EHand::Right) m_aRightSpaces.push_back({ space, bAim });
}

bool COpenXRHook::LocateHand(const std::vector<SHandSpace>& aSpaces, bool bPreferAim, XrTime time, XrPosef& outPose)
{
    if (!m_pfnLocateSpace || m_WorldSpace == XR_NULL_HANDLE) return false;
    constexpr XrSpaceLocationFlags kValid = XR_SPACE_LOCATION_POSITION_VALID_BIT | XR_SPACE_LOCATION_ORIENTATION_VALID_BIT;

    std::lock_guard<std::mutex> lock(m_SpacesLock);
    // Сначала позы нужного типа (aim / grip), потом остальные. Пространства неактивных
    // профилей контроллеров просто не отдают валидную позу, так что перебор безопасен.
    for (int iPass = 0; iPass < 2; iPass++)
        for (size_t i = aSpaces.size(); i-- > 0;)
        {
            if ((aSpaces[i].bAim == bPreferAim) != (iPass == 0)) continue;
            XrSpaceLocation loc = { XR_TYPE_SPACE_LOCATION };
            if (m_pfnLocateSpace(aSpaces[i].space, m_WorldSpace, time, &loc) == XR_SUCCESS && (loc.locationFlags & kValid) == kValid)
            {
                outPose = loc.pose;
                return true;
            }
        }
    return false;
}

void COpenXRHook::UpdateOpenAnim(float fDt)
{
    // Переключение по фронту нажатия выбранной кнопки левого контроллера.
    const auto& L = m_Input.Left;
    const bool bButton = cfg::kToggleButton == cfg::EToggleButton::X          ? L.bPrimary
                       : cfg::kToggleButton == cfg::EToggleButton::Y          ? L.bSecondary
                       : cfg::kToggleButton == cfg::EToggleButton::StickClick ? L.bStickClick
                       :                                                        L.bMenu;
    if (bButton && !m_bToggleWasDown) m_bMenuOpen = !m_bMenuOpen;
    m_bToggleWasDown = bButton;

    // Пружина: при открытии панель "выпрыгивает" из руки с лёгким перелётом (~5%),
    // при закрытии быстро схлопывается. Мелкие шаги — стабильно при любом FPS.
    const float     fTarget    = m_bMenuOpen ? 1.f : 0.f;
    constexpr float kStiffness = 260.f;
    const float     kDamping   = 2.f * 0.7f * sqrtf(kStiffness);
    for (float t = fDt; t > 0.f; t -= 1.f / 240.f)
    {
        const float h = fminf(t, 1.f / 240.f);
        m_fOpenVel += (kStiffness * (fTarget - m_fOpen) - kDamping * m_fOpenVel) * h;
        m_fOpen    += m_fOpenVel * h;
    }
    if (!m_bMenuOpen && m_fOpen <= 0.f) { m_fOpen = 0.f; m_fOpenVel = 0.f; }
    if ( m_bMenuOpen && fabsf(m_fOpen - 1.f) < 1e-4f && fabsf(m_fOpenVel) < 1e-3f) { m_fOpen = 1.f; m_fOpenVel = 0.f; }
}

void COpenXRHook::UpdatePointer(const XrPosef& quadPose, XrExtent2Df size, XrTime time, float fDt)
{
    ImGuiContext* pCtx = m_Renderer.GetImGuiContext();
    if (!pCtx) return;

    // Клики принимаются, только когда панель уже почти раскрылась и не закрывается.
    bool    bHit = false;
    float   u = 0.f, v = 0.f;
    XrPosef ray;
    if (m_bMenuOpen && m_fOpen > 0.6f && LocateHand(m_aRightSpaces, true, time, ray))
    {
        const XrVector3f dir = xrmath::RotateVectorByQuaternion(ray.orientation, { 0.f, 0.f, -1.f });
        bHit = xrmath::RaycastToQuad(ray.position, dir, quadPose, size.width, size.height, u, v);
    }

    // Курок с гистерезисом. Нажатие засчитывается, только если курок нажали, уже целясь
    // в меню, — иначе зажатым курком можно было бы "прочертить" по кнопкам.
    const float fTrigger = m_Input.Right.TriggerValue();
    const bool  bDown    = m_bTriggerDown ? (fTrigger > cfg::kTriggerRelease) : (fTrigger >= cfg::kTriggerPress);
    if (bDown && !m_bTriggerDown) m_bPressOnMenu = bHit;
    if (!bDown)                   m_bPressOnMenu = false;
    m_bTriggerDown = bDown;

    ImGui::SetCurrentContext(pCtx);
    ImGuiIO& io = ImGui::GetIO();
    // Текстура выше панели (под ней полоса спрайтов луча), поэтому — размер панели, а не DisplaySize
    io.MousePos     = bHit ? ImVec2(u * (float)cfg::kPanelResX, v * (float)cfg::kPanelResY) : ImVec2(-FLT_MAX, -FLT_MAX);
    io.MouseDown[0] = bDown && m_bPressOnMenu;

    const float fStick = m_Input.Right.fStickY;
    io.MouseWheel = (cfg::kStickScroll && bHit && fabsf(fStick) > 0.25f) ? fStick * 4.f * fDt : 0.f;

    m_bPointerFree = false;
    m_Input.m_bMaskRightHand.store(cfg::kBlockGameInputOnMenu && (bHit || m_bPressOnMenu), std::memory_order_relaxed);
    m_bPointerOnMenu.store(bHit || m_bPressOnMenu, std::memory_order_relaxed);
}

void COpenXRHook::ReleasePointer()
{
    m_bTriggerDown = false;
    m_bPressOnMenu = false;
    m_Input.m_bMaskRightHand.store(false, std::memory_order_relaxed);
    m_bPointerOnMenu.store(false, std::memory_order_relaxed);
    if (m_bPointerFree) return;

    m_bPointerFree = true;
    if (ImGuiContext* pCtx = m_Renderer.GetImGuiContext())
    {
        ImGui::SetCurrentContext(pCtx);
        ImGuiIO& io = ImGui::GetIO();
        io.MousePos     = ImVec2(-FLT_MAX, -FLT_MAX);
        io.MouseDown[0] = false;
        io.MouseWheel   = 0.f;
    }
}

const XrFrameEndInfo* COpenXRHook::OnEndFrame(XrSession session, const XrFrameEndInfo* pInfo)
{
    const uint64_t uNow = NowNs();
    float fDt = m_uLastTickNs ? (float)((double)(uNow - m_uLastTickNs) * 1e-9) : 0.f;
    m_uLastTickNs = uNow;
    if (fDt > 0.1f) fDt = 0.1f;

    UpdateOpenAnim(fDt);
    m_LastDisplayTime.store(pInfo->displayTime, std::memory_order_relaxed);

    // Луч спавна живёт, пока главный поток его обновляет (игра на паузе — луч гаснет сам)
    const bool bReady = session == m_Session && m_Renderer.IsReady() && m_WorldSpace != XR_NULL_HANDLE;
    const bool bMenu  = m_fOpen > 0.002f;
    const bool bBeam  = m_bBeamActive.load(std::memory_order_relaxed) &&
                        uNow - m_uBeamStampNs.load(std::memory_order_relaxed) < 250000000ull;

    // Шары платформ: появляются с "поп"-анимацией и так же сжимаются, когда руку отпустили
    const bool bBallFresh = uNow - m_uBallStampNs.load(std::memory_order_relaxed) < 250000000ull;
    const bool abBallOn[2] = { bBallFresh && m_bBallLeft.load(std::memory_order_relaxed),
                               bBallFresh && m_bBallRight.load(std::memory_order_relaxed) };
    for (int i = 0; i < 2; i++)
        m_afBallScale[i] += ((abBallOn[i] ? 1.f : 0.f) - m_afBallScale[i]) * (1.f - expf(-(abBallOn[i] ? 22.f : 30.f) * fDt));
    const bool bBalls = abBallOn[0] || abBallOn[1] || m_afBallScale[0] > 0.01f || m_afBallScale[1] > 0.01f;

    // Ни меню, ни луча, ни шаров (или хук ещё не готов) — кадр игры уходит без единого изменения.
    if (!bReady || (!bMenu && !bBeam && !bBalls))
    {
        ReleasePointer();
        return pInfo;
    }

    // Меню висит на левом контроллере. Позы берём на время показа ЭТОГО кадра (displayTime),
    // а не предсказание из xrWaitFrame, которое может относиться уже к следующему кадру.
    // Если рука на кадр потеряла трекинг — меню остаётся на последнем месте.
    bool        bPanel = false;
    XrExtent2Df size   = {};
    if (bMenu)
    {
        XrPosef anchor;
        if (LocateHand(m_aLeftSpaces, cfg::kAnchorToAimPose, pInfo->displayTime, anchor))
        {
            const XrPosef offset = { { 0.f, 0.f, 0.f, 1.f }, { cfg::kMenuOffset[0], cfg::kMenuOffset[1], cfg::kMenuOffset[2] } };
            m_MenuPose  = xrmath::PoseMultiply(anchor, offset);
            m_bHavePose = true;
        }
        if (m_bHavePose)
        {
            // Размер панели: из config.hpp × масштаб из настроек меню × пружина открытия.
            // Тот же размер уходит и в raycast — попадание луча остаётся точным.
            const float fScale = (m_pfnMenuScale ? m_pfnMenuScale() : 1.f) * m_fOpen;
            size   = { cfg::kPanelWidthM * fScale, cfg::kPanelHeightM * fScale };
            bPanel = true;
        }
    }

    m_Renderer.InitializeImGui(m_pfnSetupMenu);
    if (bPanel) UpdatePointer(m_MenuPose, size, pInfo->displayTime, fDt);
    else        ReleasePointer();

    // Пока луч руки на меню, луч спавна не показываем
    const bool bShowBeam = bBeam && !m_bPointerOnMenu.load(std::memory_order_relaxed);
    m_bDrawSprites = bShowBeam || bBalls;
    const float fVisibility = bPanel ? (m_fOpen < 1.f ? m_fOpen : 1.f) : 0.f;
    if (!m_Renderer.RenderFrame(pInfo->displayTime, &COpenXRHook::DrawFrame, fVisibility))
        return pInfo;

    // Слои: луч и шары под панелью, панель последней
    const XrCompositionLayerBaseHeader* apLayers[8];
    uint32_t n = 0;
    if (bShowBeam) n += (uint32_t)BuildBeamLayers(pInfo->displayTime, apLayers + n);
    if (bBalls)    n += (uint32_t)BuildBallLayers(pInfo->displayTime, apLayers + n);
    if (bPanel)
    {
        static constexpr int kPanelRect[4] = { 0, 0, cfg::kPanelResX, cfg::kPanelResY };
        apLayers[n++] = reinterpret_cast<const XrCompositionLayerBaseHeader*>(
            m_Renderer.BuildQuadLayer(&m_QuadLayer, m_WorldSpace, size, m_MenuPose, kPanelRect));
    }
    return n ? m_Composition.BuildEndInfo(pInfo, apLayers, n) : pInfo;
}

// ============================================================================
//  Главный поток игры и направление руки — для функций мода
// ============================================================================
static bool IsUnityMainThread()
{
    // Объекты Unity можно трогать только из её главного потока — на Android он называется UnityMain.
    static thread_local int s_iState = -1;
    if (s_iState < 0)
    {
        char szName[17] = {};
        prctl(PR_GET_NAME, szName, 0, 0, 0);
        s_iState = strcmp(szName, "UnityMain") == 0 ? 1 : 0;
        if (!s_iState)
            __android_log_print(ANDROID_LOG_WARN, "shkbd", "xrSyncActions on thread '%s' (not UnityMain): game mods skip it", szName);
    }
    return s_iState == 1;
}

void COpenXRHook::OnSyncActions()
{
    if (!m_pfnMainThreadTick || !IsUnityMainThread()) return;

    const uint64_t uNow = NowNs();
    float fDt = m_uLastMainTickNs ? (float)((double)(uNow - m_uLastMainTickNs) * 1e-9) : 0.f;
    m_uLastMainTickNs = uNow;
    if (fDt > 0.1f) fDt = 0.1f;
    m_pfnMainThreadTick(fDt);
}

bool COpenXRHook::GetHandInHead(bool bRight, bool bAim, XrVector3f& outDir, XrVector3f* pOutPos)
{
    const XrTime time = m_LastDisplayTime.load(std::memory_order_relaxed);
    if (!time || !m_pfnLocateSpace || m_ViewSpace == XR_NULL_HANDLE || m_WorldSpace == XR_NULL_HANDLE) return false;

    XrPosef aim;
    if (!LocateHand(bRight ? m_aRightSpaces : m_aLeftSpaces, bAim, time, aim)) return false;

    XrSpaceLocation head = { XR_TYPE_SPACE_LOCATION };
    if (m_pfnLocateSpace(m_ViewSpace, m_WorldSpace, time, &head) != XR_SUCCESS ||
        !(head.locationFlags & XR_SPACE_LOCATION_ORIENTATION_VALID_BIT) ||
        (pOutPos && !(head.locationFlags & XR_SPACE_LOCATION_POSITION_VALID_BIT)))
        return false;

    // Луч руки в мировых осях → в оси головы (поворот на обратный кватернион головы).
    const XrVector3f    dirWorld = xrmath::RotateVectorByQuaternion(aim.orientation, { 0.f, 0.f, -1.f });
    const XrQuaternionf qHeadInv = { -head.pose.orientation.x, -head.pose.orientation.y, -head.pose.orientation.z, head.pose.orientation.w };
    outDir = xrmath::RotateVectorByQuaternion(qHeadInv, dirWorld);
    if (pOutPos)
    {
        const XrVector3f d = { aim.position.x - head.pose.position.x, aim.position.y - head.pose.position.y, aim.position.z - head.pose.position.z };
        *pOutPos = xrmath::RotateVectorByQuaternion(qHeadInv, d);
    }
    return true;
}

// ============================================================================
//  Луч спавна: три quad-слоя по спрайтам из полосы под панелью
// ============================================================================
namespace
{
    XrVector3f Add (XrVector3f a, XrVector3f b) { return { a.x + b.x, a.y + b.y, a.z + b.z }; }
    XrVector3f Sub (XrVector3f a, XrVector3f b) { return { a.x - b.x, a.y - b.y, a.z - b.z }; }
    XrVector3f Mul (XrVector3f a, float s)      { return { a.x * s, a.y * s, a.z * s }; }
    float      Dot (XrVector3f a, XrVector3f b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
    XrVector3f Cross(XrVector3f a, XrVector3f b){ return { a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x }; }
    XrVector3f Normalize(XrVector3f a)          { const float l = sqrtf(Dot(a, a)); return l > 1e-6f ? Mul(a, 1.f / l) : XrVector3f{ 0.f, 0.f, 1.f }; }

    // Кватернион из ортонормированных осей (столбцы матрицы поворота)
    XrQuaternionf QuatFromAxes(XrVector3f x, XrVector3f y, XrVector3f z)
    {
        XrQuaternionf q;
        const float tr = x.x + y.y + z.z;
        if (tr > 0.f)                   { const float s = sqrtf(tr + 1.f) * 2.f;             q = { (y.z - z.y) / s, (z.x - x.z) / s, (x.y - y.x) / s, 0.25f * s }; }
        else if (x.x > y.y && x.x > z.z){ const float s = sqrtf(1.f + x.x - y.y - z.z) * 2.f; q = { 0.25f * s, (y.x + x.y) / s, (z.x + x.z) / s, (y.z - z.y) / s }; }
        else if (y.y > z.z)             { const float s = sqrtf(1.f + y.y - x.x - z.z) * 2.f; q = { (y.x + x.y) / s, 0.25f * s, (z.y + y.z) / s, (z.x - x.z) / s }; }
        else                            { const float s = sqrtf(1.f + z.z - x.x - y.y) * 2.f; q = { (z.x + x.z) / s, (z.y + y.z) / s, 0.25f * s, (x.y - y.x) / s }; }
        return xrmath::QuaternionNormalize(q);
    }

    // Плоскость лицом к глазу, "верх" — по мировой вертикали
    XrQuaternionf Billboard(XrVector3f pos, XrVector3f eye, XrVector3f& outUp)
    {
        const XrVector3f z = Normalize(Sub(eye, pos));
        XrVector3f       x = Cross({ 0.f, 1.f, 0.f }, z);
        x = Dot(x, x) < 1e-6f ? XrVector3f{ 1.f, 0.f, 0.f } : Normalize(x);
        outUp = Cross(z, x);
        return QuatFromAxes(x, outUp, z);
    }

    const XrCompositionLayerBaseHeader* Header(const XrCompositionLayerQuad* p) { return reinterpret_cast<const XrCompositionLayerBaseHeader*>(p); }
}

void COpenXRHook::SetSpawnBeam(bool bActive, float fHitM)
{
    m_fBeamHitM.store(fHitM, std::memory_order_relaxed);
    m_bBeamActive.store(bActive, std::memory_order_relaxed);
    m_uBeamStampNs.store(NowNs(), std::memory_order_relaxed);
}

void COpenXRHook::DrawFrame(float fVisibility)
{
    COpenXRHook* p = g_pOpenXRHook;
    if (!p) return;
    if (fVisibility > 0.f && p->m_pfnDrawMenu) p->m_pfnDrawMenu(fVisibility);
    if (p->m_bDrawSprites && p->m_pfnDrawSprites) p->m_pfnDrawSprites();
}

int COpenXRHook::BuildBeamLayers(XrTime time, const XrCompositionLayerBaseHeader** ppOut)
{
    XrPosef         aim;
    XrSpaceLocation head = { XR_TYPE_SPACE_LOCATION };
    if (!LocateHand(m_aRightSpaces, true, time, aim) || m_ViewSpace == XR_NULL_HANDLE ||
        m_pfnLocateSpace(m_ViewSpace, m_WorldSpace, time, &head) != XR_SUCCESS ||
        !(head.locationFlags & XR_SPACE_LOCATION_POSITION_VALID_BIT))
        return 0;

    // Поза руки — свежая, этого кадра; расстояние до попадания — из главного потока
    const XrVector3f dir  = xrmath::RotateVectorByQuaternion(aim.orientation, { 0.f, 0.f, -1.f });
    const XrVector3f eye  = head.pose.position;
    const float      fHit = m_fBeamHitM.load(std::memory_order_relaxed);
    const float      fLen = fHit >= 0.f ? (fHit > 0.01f ? fHit : 0.01f) : 5.f;
    const XrVector3f p0   = Add(aim.position, Mul(dir, 0.05f));
    const XrVector3f p1   = Add(p0, Mul(dir, fLen));

    // Луч — длинный узкий quad вдоль руки, повёрнутый плоскостью к глазу
    const XrVector3f mid = Mul(Add(p0, p1), 0.5f);
    XrVector3f       x   = Cross(dir, Normalize(Sub(eye, mid)));
    x = Dot(x, x) < 1e-6f ? Normalize(Cross(dir, { 0.f, 1.f, 0.f })) : Normalize(x);
    const XrPosef beamPose = { QuatFromAxes(x, dir, Cross(x, dir)), mid };
    int n = 0;
    ppOut[n++] = Header(m_Renderer.BuildQuadLayer(&m_aBeamLayers[0], m_WorldSpace, { cfg::kSpawnBeamWidthM, fLen }, beamPose, cfg::kBeamSprite));
    if (fHit < 0.f) return n;

    // Прицел и подпись в точке попадания: лицом к глазу, размер растёт с расстоянием
    const XrVector3f    at    = Add(p1, Mul(Normalize(Sub(eye, p1)), 0.01f));   // чуть к глазу — не тонет в стене
    const float         fDist = sqrtf(Dot(Sub(at, eye), Sub(at, eye)));
    const float         fSize = fDist * 0.04f < 0.04f ? 0.04f : (fDist * 0.04f > 1.2f ? 1.2f : fDist * 0.04f);
    XrVector3f          up;
    const XrQuaternionf q     = Billboard(at, eye, up);
    ppOut[n++] = Header(m_Renderer.BuildQuadLayer(&m_aBeamLayers[1], m_WorldSpace, { fSize, fSize }, { q, at }, cfg::kReticleSprite));

    const float fLabelW = fSize * 4.f;
    const float fLabelH = fLabelW * (float)cfg::kLabelSprite[3] / (float)cfg::kLabelSprite[2];
    const XrPosef labelPose = { q, Add(at, Mul(up, fSize * 0.6f + fLabelH * 0.5f)) };
    ppOut[n++] = Header(m_Renderer.BuildQuadLayer(&m_aBeamLayers[2], m_WorldSpace, { fLabelW, fLabelH }, labelPose, cfg::kLabelSprite));
    return n;
}

void COpenXRHook::SetGrabBalls(bool bLeft, bool bRight)
{
    m_bBallLeft.store(bLeft, std::memory_order_relaxed);
    m_bBallRight.store(bRight, std::memory_order_relaxed);
    m_uBallStampNs.store(NowNs(), std::memory_order_relaxed);
}

int COpenXRHook::BuildBallLayers(XrTime time, const XrCompositionLayerBaseHeader** ppOut)
{
    XrSpaceLocation head = { XR_TYPE_SPACE_LOCATION };
    if (m_ViewSpace == XR_NULL_HANDLE || m_pfnLocateSpace(m_ViewSpace, m_WorldSpace, time, &head) != XR_SUCCESS ||
        !(head.locationFlags & XR_SPACE_LOCATION_POSITION_VALID_BIT))
        return 0;

    // Шар — на ладони (grip) этого кадра. Мод держит руку на месте в мире игры,
    // поэтому шар вместе с рукой выглядит застрявшим в воздухе.
    int n = 0;
    for (int i = 0; i < 2; i++)
    {
        XrPosef grip;
        if (m_afBallScale[i] <= 0.01f || !LocateHand(i ? m_aRightSpaces : m_aLeftSpaces, false, time, grip)) continue;
        XrVector3f          up;
        const XrQuaternionf q    = Billboard(grip.position, head.pose.position, up);
        const float         size = cfg::kGrabBallM * m_afBallScale[i];
        ppOut[n++] = Header(m_Renderer.BuildQuadLayer(&m_aBallLayers[i], m_WorldSpace, { size, size }, { q, grip.position }, cfg::kBallSprite));
    }
    return n;
}
