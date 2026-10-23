#include "xr_composition.hpp"

const XrFrameEndInfo* COpenXRComposition::BuildEndInfo(const XrFrameEndInfo* pOrig, const XrCompositionLayerBaseHeader* const* ppExtra, uint32_t iExtraCount)
{
    this->m_aLayers.clear();
    this->m_aLayers.reserve((size_t)pOrig->layerCount + iExtraCount);
    
    if (pOrig->layerCount > 0 && pOrig->layers)
        this->m_aLayers.assign(pOrig->layers, pOrig->layers + pOrig->layerCount);
        
    for (uint32_t i = 0; i < iExtraCount; i++)
        if (ppExtra[i]) this->m_aLayers.push_back(ppExtra[i]);

    this->m_UpdatedEndInfo            = *pOrig;
    this->m_UpdatedEndInfo.layers     = this->m_aLayers.data();
    this->m_UpdatedEndInfo.layerCount = (uint32_t)this->m_aLayers.size();
    return &this->m_UpdatedEndInfo;
}
