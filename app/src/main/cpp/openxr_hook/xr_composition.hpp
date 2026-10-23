#pragma once

#include "renderer/xr_platform_includes.hpp"
#include <vector>

class COpenXRComposition
{
public:
    COpenXRComposition()  = default;
    ~COpenXRComposition() = default;

    const XrFrameEndInfo* BuildEndInfo(const XrFrameEndInfo* pOrig, const XrCompositionLayerBaseHeader* const* ppExtra, uint32_t iExtraCount);

private:
    XrFrameEndInfo                                   m_UpdatedEndInfo = {};
    std::vector<const XrCompositionLayerBaseHeader*> m_aLayers;
};
