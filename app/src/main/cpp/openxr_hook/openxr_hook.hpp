#pragma once

#include <atomic>
#include <mutex>
#include <vector>
#include "renderer/xr_platform_includes.hpp"
#include "renderer/openxr_renderer.hpp"
#include "xr_input.hpp"
#include "xr_composition.hpp"

// Перехват OpenXR игры: в кадр добавляется один quad-слой с ImGui-меню, привязанный
// к левому контроллеру; луч с правого контроллера работает как мышь.
class COpenXRHook
{
public:
    // hOpenXRLoader — хендл уже загруженного libopenxr_loader.so.
    bool Initialize(void* hOpenXRLoader);

    void (*m_pfnSetupMenu)()                 = nullptr;   // один раз: шрифт и стиль ImGui
    void (*m_pfnDrawMenu)(float fVisibility) = nullptr;   // каждый кадр, пока меню видно
    void (*m_pfnMainThreadTick)(float fDt)   = nullptr;
    float (*m_pfnMenuScale)()                = nullptr;   // множитель размера панели (настройки меню)
    void (*m_pfnDrawSprites)()               = nullptr;   // картинки луча спавна (полоса под панелью)   // главный поток игры (UnityMain), каждая синхронизация ввода

    bool IsMenuOpen() const { return m_bMenuOpen; }

    // Поза руки в осях головы (OpenXR: -Z вперёд): направление и, если нужно, положение.
    // bAim — поза луча (aim), иначе ладони (grip). false — поз пока нет.
    bool GetHandInHead(bool bRight, bool bAim, XrVector3f& outDir, XrVector3f* pOutPos = nullptr);
    bool GetRightAimInHead(XrVector3f& outDir, XrVector3f* pOutPos = nullptr) { return GetHandInHead(true, true, outDir, pOutPos); }

    // Шары платформ на руках (обновляет главный поток каждый тик)
    void SetGrabBalls(bool bLeft, bool bRight);

    // Луч спавна (обновляет главный поток каждый тик): bActive — показывать; fHitM — расстояние
    // до попадания от точки в 5 см перед рукой, в метрах; < 0 — ни во что не попал.
    void SetSpawnBeam(bool bActive, float fHitM);
    bool IsPointerOnMenu() const { return m_bPointerOnMenu.load(std::memory_order_relaxed); }

    // Кнопки контроллеров — пригодятся и функциям мода.
    COpenXRInputBinding m_Input;

    // ---- вызываются из перехватчиков ----
    void                  OnGetInstanceProcAddr(const char* pszName, PFN_xrVoidFunction* pFunc);
    void                  OnCreateSession      (XrInstance instance, const XrSessionCreateInfo* pInfo, XrSession session);
    void                  OnCreateActionSpace  (const XrActionSpaceCreateInfo* pInfo, XrSpace space);
    const XrFrameEndInfo* OnEndFrame           (XrSession session, const XrFrameEndInfo* pInfo);
    void                  OnSyncActions        ();

private:
    struct SHandSpace { XrSpace space; bool bAim; };

    bool LocateHand    (const std::vector<SHandSpace>& aSpaces, bool bPreferAim, XrTime time, XrPosef& outPose);
    void UpdateOpenAnim(float fDt);
    void UpdatePointer (const XrPosef& quadPose, XrExtent2Df size, XrTime time, float fDt);
    void ReleasePointer();
    int  BuildBeamLayers(XrTime time, const XrCompositionLayerBaseHeader** ppOut);
    int  BuildBallLayers(XrTime time, const XrCompositionLayerBaseHeader** ppOut);
    static void DrawFrame(float fVisibility);   // меню (если видно) + спрайты луча (если он включён)

    COpenXRRenderer        m_Renderer;
    COpenXRComposition     m_Composition;
    XrCompositionLayerQuad m_QuadLayer = {};
    XrCompositionLayerQuad m_aBeamLayers[3] = {};   // луч, прицел, подпись
    XrCompositionLayerQuad m_aBallLayers[2] = {};   // шары платформ: левая, правая

    PFN_xrStringToPath         m_pfnStringToPath         = nullptr;
    PFN_xrCreateReferenceSpace m_pfnCreateReferenceSpace = nullptr;
    PFN_xrLocateSpace          m_pfnLocateSpace          = nullptr;

    XrSession               m_Session    = XR_NULL_HANDLE;
    XrSpace                 m_WorldSpace = XR_NULL_HANDLE;
    XrSpace                 m_ViewSpace  = XR_NULL_HANDLE;   // голова
    std::atomic<XrTime>     m_LastDisplayTime { 0 };        // время показа последнего кадра
    uint64_t                m_uLastMainTickNs = 0;
    std::mutex              m_SpacesLock;          // xrCreateActionSpace может прийти с другого потока
    std::vector<SHandSpace> m_aLeftSpaces;
    std::vector<SHandSpace> m_aRightSpaces;

    // меню
    bool     m_bMenuOpen      = false;
    bool     m_bToggleWasDown = false;
    float    m_fOpen          = 0.f;     // пружина: масштаб панели 0..~1.05
    float    m_fOpenVel       = 0.f;
    uint64_t m_uLastTickNs    = 0;
    bool     m_bHavePose      = false;
    XrPosef  m_MenuPose       = { { 0.f, 0.f, 0.f, 1.f }, { 0.f, 0.f, 0.f } };

    // луч правой руки
    bool     m_bTriggerDown   = false;
    bool     m_bPressOnMenu   = false;
    bool     m_bPointerFree   = true;
    std::atomic<bool> m_bPointerOnMenu { false };

    // луч спавна
    std::atomic<bool>     m_bBeamActive  { false };
    std::atomic<float>    m_fBeamHitM    { -1.f };
    std::atomic<uint64_t> m_uBeamStampNs { 0 };
    bool                  m_bDrawSprites = false;

    // шары платформ
    std::atomic<bool>     m_bBallLeft    { false };
    std::atomic<bool>     m_bBallRight   { false };
    std::atomic<uint64_t> m_uBallStampNs { 0 };
    float                 m_afBallScale[2] = { 0.f, 0.f };   // анимация появления, 0..1
};

extern COpenXRHook* g_pOpenXRHook;
