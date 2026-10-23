#include "openxr_renderer.hpp"
#include "vulkan_backend.hpp"
#include "opengl_backend.hpp"

#include <float.h>
#include <vector>
#include "imgui.h"

bool COpenXRRenderer::Bind(XrSession session, const XrBaseInStructure* pBinding, const SXrSwapchainApi& api, int iWidth, int iHeight)
{
    // Новая сессия. Объекты прошлой не трогаем: её swapchain рантайм уничтожил вместе с ней,
    // а графическое устройство игры могло смениться — старые бэкенд и контекст просто отпускаем.
    m_Api             = api;
    m_pBackend        = nullptr;
    m_Swapchain       = XR_NULL_HANDLE;
    m_pCtx            = nullptr;
    m_bImGuiInit      = false;
    m_LastDisplayTime = 0;
    m_iWidth          = iWidth;
    m_iHeight         = iHeight;

    if (!pBinding || !api.IsValid()) return false;

    const bool bVulkan = pBinding->type == XR_TYPE_GRAPHICS_BINDING_VULKAN_KHR;
    IRenderBackend* pBackend = bVulkan ? static_cast<IRenderBackend*>(new CVulkanBackend)
                                       : static_cast<IRenderBackend*>(new COpenGLBackend);

    const int64_t iFormat = PickFormat(session, pBackend);
    if (!pBackend->Initialize(pBinding, iWidth, iHeight, iFormat))
    {
        delete pBackend;
        return false;
    }

    XrSwapchainCreateInfo info = { XR_TYPE_SWAPCHAIN_CREATE_INFO };
    info.usageFlags  = XR_SWAPCHAIN_USAGE_COLOR_ATTACHMENT_BIT;
    info.format      = iFormat;
    info.sampleCount = 1;
    info.width       = (uint32_t)iWidth;
    info.height      = (uint32_t)iHeight;
    info.faceCount   = 1;
    info.arraySize   = 1;
    info.mipCount    = 1;

    XrSwapchain swapchain = XR_NULL_HANDLE;
    if (api.pfnCreate(session, &info, &swapchain) != XR_SUCCESS || swapchain == XR_NULL_HANDLE)
    {
        delete pBackend;
        return false;
    }

    uint32_t nImages = 0;
    bool bOk = api.pfnEnumerateImages(swapchain, 0, &nImages, nullptr) == XR_SUCCESS && nImages > 0;
    if (bOk && bVulkan)
    {
        std::vector<XrSwapchainImageVulkanKHR> aImages(nImages, { XR_TYPE_SWAPCHAIN_IMAGE_VULKAN_KHR });
        bOk = api.pfnEnumerateImages(swapchain, nImages, &nImages, reinterpret_cast<XrSwapchainImageBaseHeader*>(aImages.data())) == XR_SUCCESS
           && pBackend->OnSwapchainImages(aImages.data(), nImages);
    }
    else if (bOk)
    {
        std::vector<XrSwapchainImageOpenGLESKHR> aImages(nImages, { XR_TYPE_SWAPCHAIN_IMAGE_OPENGL_ES_KHR });
        bOk = api.pfnEnumerateImages(swapchain, nImages, &nImages, reinterpret_cast<XrSwapchainImageBaseHeader*>(aImages.data())) == XR_SUCCESS
           && pBackend->OnSwapchainImages(aImages.data(), nImages);
    }

    if (!bOk)
    {
        if (api.pfnDestroy) api.pfnDestroy(swapchain);
        delete pBackend;
        return false;
    }

    m_pBackend  = pBackend;
    m_Swapchain = swapchain;
    return true;
}

int64_t COpenXRRenderer::PickFormat(XrSession session, const IRenderBackend* pBackend) const
{
    // Первый формат из списка бэкенда (sRGB — первыми), который поддерживает рантайм.
    uint32_t       nPreferred = 0;
    const int64_t* pPreferred = pBackend->PreferredFormats(nPreferred);

    uint32_t nFormats = 0;
    if (m_Api.pfnEnumerateFormats && m_Api.pfnEnumerateFormats(session, 0, &nFormats, nullptr) == XR_SUCCESS && nFormats > 0)
    {
        std::vector<int64_t> aFormats(nFormats);
        if (m_Api.pfnEnumerateFormats(session, nFormats, &nFormats, aFormats.data()) == XR_SUCCESS)
            for (uint32_t i = 0; i < nPreferred; i++)
                for (uint32_t j = 0; j < nFormats; j++)
                    if (aFormats[j] == pPreferred[i]) return pPreferred[i];
    }
    return pPreferred[0];
}

void COpenXRRenderer::InitializeImGui(void (*pfnSetup)())
{
    if (m_bImGuiInit || !IsReady()) return;
    m_bImGuiInit = true;   // одна попытка на сессию, без повторов каждый кадр

    m_pCtx = ImGui::CreateContext();
    ImGui::SetCurrentContext(m_pCtx);

    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = nullptr;   // не пытаться писать imgui.ini и лог в чужую папку
    io.LogFilename = nullptr;
    io.DisplaySize = ImVec2((float)m_iWidth, (float)m_iHeight);
    io.MousePos    = ImVec2(-FLT_MAX, -FLT_MAX);

    if (!m_pBackend->InitImGui())
    {
        ImGui::DestroyContext(m_pCtx);
        m_pCtx = nullptr;
        return;
    }
    if (pfnSetup) pfnSetup();
}

bool COpenXRRenderer::RenderFrame(XrTime displayTime, void (*pfnDraw)(float), float fVisibility)
{
    if (!m_pCtx || !IsReady()) return false;

    uint32_t iImage = 0;
    XrSwapchainImageAcquireInfo acquire = { XR_TYPE_SWAPCHAIN_IMAGE_ACQUIRE_INFO };
    if (m_Api.pfnAcquire(m_Swapchain, &acquire, &iImage) != XR_SUCCESS) return false;

    XrSwapchainImageWaitInfo wait = { XR_TYPE_SWAPCHAIN_IMAGE_WAIT_INFO };
    wait.timeout = XR_INFINITE_DURATION;
    bool bOk = m_Api.pfnWait(m_Swapchain, &wait) == XR_SUCCESS;

    if (bOk)
    {
        ImGui::SetCurrentContext(m_pCtx);
        ImGuiIO& io = ImGui::GetIO();
        io.DisplaySize = ImVec2((float)m_iWidth, (float)m_iHeight);

        // Шаг времени — по времени показа кадров. После паузы (меню было закрыто) — обычный кадр.
        const float fDt = (m_LastDisplayTime != 0 && displayTime > m_LastDisplayTime)
                        ? (float)((double)(displayTime - m_LastDisplayTime) * 1e-9) : 0.f;
        m_LastDisplayTime = displayTime;
        io.DeltaTime = (fDt > 0.f && fDt <= 0.1f) ? fDt : 1.f / 72.f;

        bOk = m_pBackend->BeginFrame(iImage);
        if (bOk)
        {
            ImGui::NewFrame();
            if (pfnDraw) pfnDraw(fVisibility);
            ImGui::Render();
            m_pBackend->EndFrame();
        }
    }

    XrSwapchainImageReleaseInfo release = { XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO };
    m_Api.pfnRelease(m_Swapchain, &release);
    return bOk;
}

const XrCompositionLayerQuad* COpenXRRenderer::BuildQuadLayer(XrCompositionLayerQuad* pLayer, XrSpace space,
                                                              XrExtent2Df size, const XrPosef& pose, const int rect[4]) const
{
    *pLayer = { XR_TYPE_COMPOSITION_LAYER_QUAD };
    // ImGui рисует поверх прозрачного фона — альфа в текстуре уже премультиплицирована.
    pLayer->layerFlags               = XR_COMPOSITION_LAYER_BLEND_TEXTURE_SOURCE_ALPHA_BIT;
    pLayer->space                    = space;
    pLayer->eyeVisibility            = XR_EYE_VISIBILITY_BOTH;
    pLayer->subImage.swapchain       = m_Swapchain;
    pLayer->subImage.imageRect       = { { rect[0], rect[1] }, { rect[2], rect[3] } };
    pLayer->subImage.imageArrayIndex = 0;
    pLayer->pose                     = pose;
    pLayer->size                     = size;
    return pLayer;
}
