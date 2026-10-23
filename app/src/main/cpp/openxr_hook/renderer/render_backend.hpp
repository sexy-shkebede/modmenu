#pragma once

#include <stdint.h>
#include "xr_platform_includes.hpp"

// Графический бэкенд swapchain'а меню: Vulkan или GLES — смотря чем рисует игра.
class IRenderBackend
{
public:
    virtual ~IRenderBackend() = default;

    // Форматы swapchain в порядке предпочтения (sRGB — первыми).
    virtual const int64_t* PreferredFormats(uint32_t& nCount) const = 0;

    virtual bool Initialize       (const XrBaseInStructure* pBinding, int iWidth, int iHeight, int64_t iFormat) = 0;
    virtual bool OnSwapchainImages(const void* pImages, uint32_t nCount) = 0;
    virtual bool InitImGui        () = 0;
    virtual bool BeginFrame       (uint32_t iImage) = 0;   // false — кадр пропускается
    virtual void EndFrame         () = 0;
};
