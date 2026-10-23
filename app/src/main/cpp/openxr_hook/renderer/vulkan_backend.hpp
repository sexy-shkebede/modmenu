#pragma once

#include <vector>
#include "render_backend.hpp"

class CVulkanBackend final : public IRenderBackend
{
public:
    ~CVulkanBackend() override;

    const int64_t* PreferredFormats(uint32_t& nCount) const override;

    bool Initialize       (const XrBaseInStructure* pBinding, int iWidth, int iHeight, int64_t iFormat) override;
    bool OnSwapchainImages(const void* pImages, uint32_t nCount) override;
    bool InitImGui        () override;
    bool BeginFrame       (uint32_t iImage) override;
    void EndFrame         () override;

private:
    struct SImage { VkImageView view = VK_NULL_HANDLE; VkFramebuffer fb = VK_NULL_HANDLE; };
    struct SSlot  { VkCommandBuffer cmd = VK_NULL_HANDLE; VkFence fence = VK_NULL_HANDLE; };

    VkInstance       m_Instance    = VK_NULL_HANDLE;
    VkPhysicalDevice m_Phys        = VK_NULL_HANDLE;
    VkDevice         m_Dev         = VK_NULL_HANDLE;
    VkQueue          m_Queue       = VK_NULL_HANDLE;
    uint32_t         m_QueueFamily = 0;
    VkFormat         m_Format      = VK_FORMAT_UNDEFINED;
    int              m_iWidth      = 0;
    int              m_iHeight     = 0;

    VkRenderPass     m_RenderPass  = VK_NULL_HANDLE;
    VkCommandPool    m_CmdPool     = VK_NULL_HANDLE;

    std::vector<SImage> m_aImages;   // на каждую картинку swapchain: view + framebuffer
    std::vector<SSlot>  m_aSlots;    // кадры "в полёте": у каждого свой command buffer и fence
    uint32_t            m_iSlot = 0;
};
