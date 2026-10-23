#include "vulkan_backend.hpp"

#include "imgui.h"
#include "imgui_impl_vulkan.h"

const int64_t* CVulkanBackend::PreferredFormats(uint32_t& nCount) const
{
    static const int64_t kFormats[] = { VK_FORMAT_R8G8B8A8_SRGB, VK_FORMAT_B8G8R8A8_SRGB, VK_FORMAT_R8G8B8A8_UNORM };
    nCount = (uint32_t)(sizeof(kFormats) / sizeof(kFormats[0]));
    return kFormats;
}

bool CVulkanBackend::Initialize(const XrBaseInStructure* pBinding, int iWidth, int iHeight, int64_t iFormat)
{
    const auto* pVk = reinterpret_cast<const XrGraphicsBindingVulkanKHR*>(pBinding);
    m_Instance    = pVk->instance;
    m_Phys        = pVk->physicalDevice;
    m_Dev         = pVk->device;
    m_QueueFamily = pVk->queueFamilyIndex;
    m_Format      = (VkFormat)iFormat;
    m_iWidth      = iWidth;
    m_iHeight     = iHeight;
    vkGetDeviceQueue(m_Dev, pVk->queueFamilyIndex, pVk->queueIndex, &m_Queue);

    VkAttachmentDescription attachment = {};
    attachment.format         = m_Format;
    attachment.samples        = VK_SAMPLE_COUNT_1_BIT;
    attachment.loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;               // чистим в прозрачный (0,0,0,0)
    attachment.storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
    attachment.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachment.initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
    attachment.finalLayout    = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;  // так картинку ждёт рантайм

    VkAttachmentReference colorRef = { 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };
    VkSubpassDescription  subpass  = {};
    subpass.pipelineBindPoint    = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments    = &colorRef;

    VkRenderPassCreateInfo rpInfo = { VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO };
    rpInfo.attachmentCount = 1;
    rpInfo.pAttachments    = &attachment;
    rpInfo.subpassCount    = 1;
    rpInfo.pSubpasses      = &subpass;
    if (vkCreateRenderPass(m_Dev, &rpInfo, nullptr, &m_RenderPass) != VK_SUCCESS) return false;

    VkCommandPoolCreateInfo poolInfo = { VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
    poolInfo.flags            = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.queueFamilyIndex = m_QueueFamily;
    return vkCreateCommandPool(m_Dev, &poolInfo, nullptr, &m_CmdPool) == VK_SUCCESS;
}

bool CVulkanBackend::OnSwapchainImages(const void* pImages, uint32_t nCount)
{
    const auto* pVkImages = static_cast<const XrSwapchainImageVulkanKHR*>(pImages);

    m_aImages.resize(nCount);
    for (uint32_t i = 0; i < nCount; i++)
    {
        VkImageViewCreateInfo viewInfo = { VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
        viewInfo.image            = pVkImages[i].image;
        viewInfo.viewType         = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format           = m_Format;
        viewInfo.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
        if (vkCreateImageView(m_Dev, &viewInfo, nullptr, &m_aImages[i].view) != VK_SUCCESS) return false;

        VkFramebufferCreateInfo fbInfo = { VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO };
        fbInfo.renderPass      = m_RenderPass;
        fbInfo.attachmentCount = 1;
        fbInfo.pAttachments    = &m_aImages[i].view;
        fbInfo.width           = (uint32_t)m_iWidth;
        fbInfo.height          = (uint32_t)m_iHeight;
        fbInfo.layers          = 1;
        if (vkCreateFramebuffer(m_Dev, &fbInfo, nullptr, &m_aImages[i].fb) != VK_SUCCESS) return false;
    }

    // Сколько кадров может быть "в полёте", столько и слотов (минимум 2). Раньше здесь был
    // vkQueueWaitIdle каждый кадр: поток игры стоял, пока GPU не дорисует вообще всё.
    m_aSlots.resize(nCount < 2 ? 2 : nCount);

    VkCommandBufferAllocateInfo allocInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
    allocInfo.commandPool        = m_CmdPool;
    allocInfo.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = 1;

    VkFenceCreateInfo fenceInfo = { VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;   // первый кадр каждого слота ничего не ждёт

    for (SSlot& slot : m_aSlots)
    {
        if (vkAllocateCommandBuffers(m_Dev, &allocInfo, &slot.cmd) != VK_SUCCESS) return false;
        if (vkCreateFence(m_Dev, &fenceInfo, nullptr, &slot.fence) != VK_SUCCESS) return false;
    }
    return nCount > 0;
}

bool CVulkanBackend::InitImGui()
{
    ImGui_ImplVulkan_InitInfo info = {};
    info.Instance       = m_Instance;
    info.PhysicalDevice = m_Phys;
    info.Device         = m_Dev;
    info.QueueFamily    = m_QueueFamily;
    info.Queue          = m_Queue;
    // Пул дескрипторов создаёт сам бэкенд ImGui — с типами, которые нужны версии 1.92
    // (SAMPLED_IMAGE + SAMPLER), и с флагом освобождения: атлас шрифтов растёт и пересоздаётся.
    info.DescriptorPoolSize           = IMGUI_IMPL_VULKAN_MINIMUM_SAMPLED_IMAGE_POOL_SIZE;
    info.MinImageCount                = 2;
    info.ImageCount                   = (uint32_t)m_aSlots.size();
    info.PipelineInfoMain.RenderPass  = m_RenderPass;
    info.PipelineInfoMain.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
    return ImGui_ImplVulkan_Init(&info);
}

bool CVulkanBackend::BeginFrame(uint32_t iImage)
{
    if (iImage >= m_aImages.size() || m_aSlots.empty()) return false;

    // Ждём кадр, который этот слот отправлял N кадров назад. Обычно он давно готов — ожидания нет.
    SSlot& slot = m_aSlots[m_iSlot];
    if (vkWaitForFences(m_Dev, 1, &slot.fence, VK_TRUE, 1000000000ull) != VK_SUCCESS) return false;
    if (vkResetCommandBuffer(slot.cmd, 0) != VK_SUCCESS) return false;

    VkCommandBufferBeginInfo beginInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    if (vkBeginCommandBuffer(slot.cmd, &beginInfo) != VK_SUCCESS) return false;

    VkClearValue clear = {};   // прозрачный фон
    VkRenderPassBeginInfo rpBegin = { VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO };
    rpBegin.renderPass        = m_RenderPass;
    rpBegin.framebuffer       = m_aImages[iImage].fb;
    rpBegin.renderArea.extent = { (uint32_t)m_iWidth, (uint32_t)m_iHeight };
    rpBegin.clearValueCount   = 1;
    rpBegin.pClearValues      = &clear;
    vkCmdBeginRenderPass(slot.cmd, &rpBegin, VK_SUBPASS_CONTENTS_INLINE);

    ImGui_ImplVulkan_NewFrame();
    return true;
}

void CVulkanBackend::EndFrame()
{
    SSlot& slot = m_aSlots[m_iSlot];
    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), slot.cmd);
    vkCmdEndRenderPass(slot.cmd);
    vkEndCommandBuffer(slot.cmd);

    vkResetFences(m_Dev, 1, &slot.fence);
    VkSubmitInfo submit = { VK_STRUCTURE_TYPE_SUBMIT_INFO };
    submit.commandBufferCount = 1;
    submit.pCommandBuffers    = &slot.cmd;
    if (vkQueueSubmit(m_Queue, 1, &submit, slot.fence) != VK_SUCCESS)
    {
        // Сабмит не прошёл — fence остался бы несигнальным, и следующий кадр ждал бы его вечно.
        vkDestroyFence(m_Dev, slot.fence, nullptr);
        VkFenceCreateInfo fenceInfo = { VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
        fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
        vkCreateFence(m_Dev, &fenceInfo, nullptr, &slot.fence);
    }
    m_iSlot = (m_iSlot + 1) % (uint32_t)m_aSlots.size();
}

CVulkanBackend::~CVulkanBackend()
{
    // Удаляется только если бэкенд не удалось поднять до конца — в полёте у него ничего нет.
    if (m_Dev == VK_NULL_HANDLE) return;
    for (SSlot& slot : m_aSlots)
        if (slot.fence != VK_NULL_HANDLE) vkDestroyFence(m_Dev, slot.fence, nullptr);
    if (m_CmdPool != VK_NULL_HANDLE) vkDestroyCommandPool(m_Dev, m_CmdPool, nullptr);
    for (SImage& img : m_aImages)
    {
        if (img.fb   != VK_NULL_HANDLE) vkDestroyFramebuffer(m_Dev, img.fb, nullptr);
        if (img.view != VK_NULL_HANDLE) vkDestroyImageView(m_Dev, img.view, nullptr);
    }
    if (m_RenderPass != VK_NULL_HANDLE) vkDestroyRenderPass(m_Dev, m_RenderPass, nullptr);
}
