#include "opengl_backend.hpp"

#include <string.h>
#include "imgui.h"
#include "imgui_impl_opengl3.h"

#ifndef GL_FRAMEBUFFER_SRGB_EXT
#define GL_FRAMEBUFFER_SRGB_EXT 0x8DB9
#endif

const int64_t* COpenGLBackend::PreferredFormats(uint32_t& nCount) const
{
    // sRGB первым: RGBA8 рантайм считает линейным, и тёмные тона меню уходили в "ступеньки".
    static const int64_t kFormats[] = { GL_SRGB8_ALPHA8, GL_RGBA8 };
    nCount = 2;
    return kFormats;
}

bool COpenGLBackend::Initialize(const XrBaseInStructure*, int, int, int64_t)
{
    // GL-объекты создаются позже, в BeginFrame: там гарантированно текущий контекст игры.
    return true;
}

bool COpenGLBackend::OnSwapchainImages(const void* pImages, uint32_t nCount)
{
    const auto* pGlImages = static_cast<const XrSwapchainImageOpenGLESKHR*>(pImages);
    m_aImages.resize(nCount);
    for (uint32_t i = 0; i < nCount; i++) m_aImages[i] = pGlImages[i].image;
    return nCount > 0;
}

bool COpenGLBackend::InitImGui()
{
    return ImGui_ImplOpenGL3_Init("#version 300 es");
}

bool COpenGLBackend::BeginFrame(uint32_t iImage)
{
    if (iImage >= m_aImages.size()) return false;

    if (!m_bGlReady)
    {
        m_bGlReady = true;
        glGenFramebuffers(1, &m_Fbo);
        const char* pszExt = reinterpret_cast<const char*>(glGetString(GL_EXTENSIONS));
        m_bSrgbWriteCtl = pszExt && strstr(pszExt, "GL_EXT_sRGB_write_control");
    }
    if (!m_Fbo) return false;

    glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &m_iSavedDrawFbo);
    glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &m_iSavedReadFbo);
    glBindFramebuffer(GL_FRAMEBUFFER, m_Fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_aImages[iImage], 0);

    // Запись в sRGB-текстуру должна кодироваться (игра могла это выключить).
    if (m_bSrgbWriteCtl)
    {
        m_bSavedSrgbWrite = glIsEnabled(GL_FRAMEBUFFER_SRGB_EXT);
        glEnable(GL_FRAMEBUFFER_SRGB_EXT);
    }

    // Чистим в прозрачный цвет, что бы ни оставила игра (scissor, маска цвета).
    glGetBooleanv(GL_COLOR_WRITEMASK, m_aSavedMask);
    GLfloat aClear[4];
    glGetFloatv(GL_COLOR_CLEAR_VALUE, aClear);
    const GLboolean bScissor = glIsEnabled(GL_SCISSOR_TEST);

    glDisable(GL_SCISSOR_TEST);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);   // остаётся до конца отрисовки ImGui
    glClearColor(0.f, 0.f, 0.f, 0.f);
    glClear(GL_COLOR_BUFFER_BIT);
    glClearColor(aClear[0], aClear[1], aClear[2], aClear[3]);
    if (bScissor) glEnable(GL_SCISSOR_TEST);

    ImGui_ImplOpenGL3_NewFrame();
    return true;
}

void COpenGLBackend::EndFrame()
{
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    glColorMask(m_aSavedMask[0], m_aSavedMask[1], m_aSavedMask[2], m_aSavedMask[3]);
    if (m_bSrgbWriteCtl && !m_bSavedSrgbWrite) glDisable(GL_FRAMEBUFFER_SRGB_EXT);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, (GLuint)m_iSavedDrawFbo);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, (GLuint)m_iSavedReadFbo);
}
