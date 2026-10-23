#pragma once

#include "xr_platform_includes.hpp"
#include "render_backend.hpp"

struct ImGuiContext;

// Функции рантайма для работы со swapchain (хук берёт их напрямую у загрузчика).
struct SXrSwapchainApi
{
    PFN_xrEnumerateSwapchainFormats pfnEnumerateFormats = nullptr;
    PFN_xrCreateSwapchain           pfnCreate           = nullptr;
    PFN_xrDestroySwapchain          pfnDestroy          = nullptr;
    PFN_xrEnumerateSwapchainImages  pfnEnumerateImages  = nullptr;
    PFN_xrAcquireSwapchainImage     pfnAcquire          = nullptr;
    PFN_xrWaitSwapchainImage        pfnWait             = nullptr;
    PFN_xrReleaseSwapchainImage     pfnRelease          = nullptr;

    bool IsValid() const { return pfnCreate && pfnEnumerateImages && pfnAcquire && pfnWait && pfnRelease; }
};

// Swapchain + графический бэкенд + свой ImGui-контекст: панель меню или слой мира.
class COpenXRRenderer
{
public:
    bool Bind(XrSession session, const XrBaseInStructure* pBinding, const SXrSwapchainApi& api, int iWidth, int iHeight);
    bool IsReady() const { return m_pBackend != nullptr && m_Swapchain != XR_NULL_HANDLE; }
    XrSwapchain GetSwapchain() const { return m_Swapchain; }

    void          InitializeImGui(void (*pfnSetup)());
    ImGuiContext* GetImGuiContext() const { return m_pCtx; }

    // Рисует кадр ImGui в очередную картинку swapchain. false — кадр не готов, слой не добавлять.
    bool RenderFrame(XrTime displayTime, void (*pfnDraw)(float), float fVisibility);

    // Quad-слой по участку текстуры rect = { x, y, ширина, высота } (панель или спрайт луча)
    const XrCompositionLayerQuad* BuildQuadLayer(XrCompositionLayerQuad* pLayer, XrSpace space,
                                                 XrExtent2Df size, const XrPosef& pose, const int rect[4]) const;

private:
    int64_t PickFormat(XrSession session, const IRenderBackend* pBackend) const;

    SXrSwapchainApi m_Api;
    IRenderBackend* m_pBackend        = nullptr;
    XrSwapchain     m_Swapchain       = XR_NULL_HANDLE;
    ImGuiContext*   m_pCtx            = nullptr;
    bool            m_bImGuiInit      = false;
    int             m_iWidth          = 0;
    int             m_iHeight         = 0;
    XrTime          m_LastDisplayTime = 0;
};
