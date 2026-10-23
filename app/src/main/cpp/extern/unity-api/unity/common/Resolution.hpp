#pragma once

#include <cstdint>
struct SResolution
{
    int32_t m_iWidth;
    int32_t m_iHeight;
    
    SResolution() = default;
    ~SResolution() = default;

    SResolution(int32_t w, int32_t h)
    {
        this->m_iWidth = w;
        this->m_iHeight = h;
    }
};
