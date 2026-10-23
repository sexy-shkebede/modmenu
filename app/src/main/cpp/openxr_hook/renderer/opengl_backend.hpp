#pragma once

#include <vector>
#include "render_backend.hpp"

class COpenGLBackend final : public IRenderBackend
{
public:
    const int64_t* PreferredFormats(uint32_t& nCount) const override;

    bool Initialize       (const XrBaseInStructure* pBinding, int iWidth, int iHeight, int64_t iFormat) override;
    bool OnSwapchainImages(const void* pImages, uint32_t nCount) override;
    bool InitImGui        () override;
    bool BeginFrame       (uint32_t iImage) override;
    void EndFrame         () override;

private:
    std::vector<GLuint> m_aImages;
    GLuint    m_Fbo             = 0;
    bool      m_bGlReady        = false;
    bool      m_bSrgbWriteCtl   = false;   // есть GL_EXT_sRGB_write_control
    GLboolean m_bSavedSrgbWrite = GL_TRUE;
    GLint     m_iSavedDrawFbo   = 0;
    GLint     m_iSavedReadFbo   = 0;
    GLboolean m_aSavedMask[4]   = { GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE };
};
