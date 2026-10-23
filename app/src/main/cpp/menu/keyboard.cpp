#include "keyboard.hpp"
#include "ui.hpp"

#include "imgui.h"
#include "imgui_internal.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

using namespace ui;

namespace
{
    char             g_szTitle[32] = {};
    char             g_szText[96]  = {};
    int              g_nMax    = 0;
    bool             g_bOpen   = false;
    bool             g_bShift  = false;
    bool             g_bFocus  = false;
    float            g_fAnim   = 0.f;
    keyboard::FnDone g_pfnDone = nullptr;

    constexpr float kSheetH = dp(432);

    enum class EKey { Char, Shift, Backspace, Space };

    bool Key(const char* pszId, const ImRect& bb, const char* pszLabel, EKey eKind)
    {
        ImGuiWindow*  win = ImGui::GetCurrentWindow();
        const ImGuiID id  = win->GetID(pszId);
        ImGui::ItemAdd(bb, id);
        bool bHovered = false, bHeld = false;
        const bool bPressed = ImGui::ButtonBehavior(bb, id, &bHovered, &bHeld, ImGuiButtonFlags_PressedOnClick);
        float*     pState   = ImGui::GetStateStorage()->GetFloatRef(id, 0.f);
        *pState = Approach(*pState, bHeld ? 1.25f : (bHovered ? 1.f : 0.f), 25.f, AnimDt());   // 1.25 × 8 % = 10 %

        const bool     bSpecial = eKind == EKey::Shift || eKind == EKey::Backspace;
        const bool     bActive  = eKind == EKey::Shift && g_bShift;
        const unsigned bg = bActive ? role::Primary   : bSpecial ? role::SecondaryContainer   : role::SurfaceContainerHighest;
        const unsigned fg = bActive ? role::OnPrimary : bSpecial ? role::OnSecondaryContainer : role::OnSurface;
        ImDrawList*    dl = win->DrawList;
        const float    r  = dp(12);
        RoundRect(dl, bb.Min, bb.Max, r, r, r, r, Col(Mix(bg, fg, state::Hover * *pState)));

        const ImVec2 c = bb.GetCenter();
        if (eKind == EKey::Shift)          Icon(dl, g_bShift ? EIcon::ShiftOn : EIcon::Shift, c, dp(24), Col(fg));
        else if (eKind == EKey::Backspace) Icon(dl, EIcon::Backspace, c, dp(24), Col(fg));
        else
        {
            const ImVec2 ts = TextSize(type::BodyLarge, pszLabel);
            Text(dl, type::BodyLarge, ImVec2(c.x - ts.x * 0.5f, c.y - type::BodyLarge * 0.5f), Col(fg), pszLabel);
        }
        return bPressed;
    }

    void Type(char ch)
    {
        const size_t n = strlen(g_szText);
        if ((int)n >= g_nMax) return;
        if (g_bShift && ch >= 'a' && ch <= 'z') ch = (char)(ch - 'a' + 'A');
        g_szText[n]     = ch;
        g_szText[n + 1] = 0;
        g_bShift = false;   // как на телефоне: Shift — на одну букву
    }

    void KeyRow(const char* pszChars, float x, float y, float u, float h, float gap)
    {
        for (const char* p = pszChars; *p; p++, x += u + gap)
        {
            const char szLabel[2] = { g_bShift && *p >= 'a' && *p <= 'z' ? (char)(*p - 'a' + 'A') : *p, 0 };
            const char szId[3]    = { 'k', *p, 0 };
            if (Key(szId, ImRect(ImVec2(x, y), ImVec2(x + u, y + h)), szLabel, EKey::Char))
                Type(*p);
        }
    }
}

void keyboard::Open(const char* pszTitle, const char* pszInitial, int nMaxLen, FnDone pfnDone)
{
    snprintf(g_szTitle, sizeof(g_szTitle), "%s", pszTitle);
    g_nMax = nMaxLen < (int)sizeof(g_szText) - 1 ? nMaxLen : (int)sizeof(g_szText) - 1;
    snprintf(g_szText, (size_t)g_nMax + 1, "%s", pszInitial ? pszInitial : "");
    g_pfnDone = pfnDone;
    g_bOpen   = true;
    g_bShift  = false;
    g_bFocus  = true;
}

bool keyboard::IsOpen() { return g_bOpen || g_fAnim > 0.f; }

void keyboard::Draw(float W, float H)
{
    g_fAnim = MoveTowards(g_fAnim, g_bOpen ? 1.f : 0.f, AnimDt() / (g_bOpen ? dur::Medium2 : dur::Short4));
    if (g_fAnim <= 0.f) return;
    const float t = EmphasizedDecelerate(g_fAnim);

    ImGui::SetNextWindowPos(ImVec2(0.f, 0.f));
    ImGui::SetNextWindowSize(ImVec2(W, H));
    if (g_bFocus) { ImGui::SetNextWindowFocus(); g_bFocus = false; }
    ImGui::Begin("##keyboard", nullptr,
                 ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings |
                 ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoNav);
    ImDrawList* dl = ImGui::GetWindowDrawList();
    const float R  = shape::ExtraLarge;

    // Scrim M3 (32 %) над меню; нажатие по нему — отмена
    RoundRect(dl, ImVec2(0.f, 0.f), ImVec2(W, H), R, R, R, R, Col(0x000000, 0.32f * t));
    const float y0 = H - kSheetH * t;
    ImGui::SetCursorScreenPos(ImVec2(0.f, 0.f));
    if (ImGui::InvisibleButton("##scrim", ImVec2(W, ImMax(1.f, y0)), ImGuiButtonFlags_PressedOnClick) && g_bOpen)
        g_bOpen = false;

    // Лист: surface container low, углы extra large; ручка 32×4 dp
    RoundRect(dl, ImVec2(0.f, y0), ImVec2(W, y0 + kSheetH), R, R, R, R, Col(role::SurfaceContainerLow));
    RoundRect(dl, ImVec2(W * 0.5f - dp(16), y0 + dp(10)), ImVec2(W * 0.5f + dp(16), y0 + dp(14)), dp(2), dp(2), dp(2), dp(2), Col(role::OnSurfaceVariant, 0.4f));

    // Шапка: заголовок, Cancel (text button) и Save (filled button)
    const float hy      = y0 + dp(44);
    const float wSave   = TextSize(type::LabelLarge, "Save").x + dp(48);
    const float wCancel = TextSize(type::LabelLarge, "Cancel").x + dp(24);
    Text(dl, type::TitleLarge, ImVec2(dp(24), hy - type::TitleLarge * 0.5f), Col(role::OnSurface), g_szTitle);
    ImGui::SetCursorScreenPos(ImVec2(W - dp(24) - wSave, hy - dp(20)));
    if (Button("Save", EButton::Filled, wSave) && g_bOpen)
    {
        if (g_pfnDone) g_pfnDone(g_szText);
        g_bOpen = false;
    }
    ImGui::SetCursorScreenPos(ImVec2(W - dp(24) - wSave - dp(8) - wCancel, hy - dp(20)));
    if (Button("Cancel", EButton::Text, wCancel) && g_bOpen)
        g_bOpen = false;

    // Текстовое поле M3 (filled): контейнер, подпись primary, активный индикатор снизу, курсор
    const ImVec2 f0(dp(24), y0 + dp(80)), f1(W - dp(24), y0 + dp(136));
    RoundRect(dl, f0, f1, shape::ExtraSmall, shape::ExtraSmall, 0.f, 0.f, Col(role::SurfaceContainerHighest));
    dl->AddRectFilled(ImVec2(f0.x, f1.y - dp(2)), f1, Col(role::Primary));
    Text(dl, type::BodySmall, ImVec2(f0.x + dp(16), f0.y + dp(8)), Col(role::Primary), g_szTitle);
    const ImVec2 tp(f0.x + dp(16), f0.y + dp(24));
    Text(dl, type::BodyLarge, tp, Col(role::OnSurface), g_szText);
    if (fmodf((float)ImGui::GetTime(), 1.f) < 0.55f)
    {
        const float cx = tp.x + TextSize(type::BodyLarge, g_szText).x + dp(1);
        dl->AddRectFilled(ImVec2(cx, tp.y), ImVec2(cx + dp(2), tp.y + type::BodyLarge), Col(role::Primary));
    }
    char szCount[16];
    snprintf(szCount, sizeof(szCount), "%d / %d", (int)strlen(g_szText), g_nMax);
    const ImVec2 cs = TextSize(type::BodySmall, szCount);
    Text(dl, type::BodySmall, ImVec2(f1.x - dp(16) - cs.x, f1.y + dp(4)), Col(role::OnSurfaceVariant), szCount);

    // Раскладка: 10 клавиш в ряду
    const float x0 = dp(24), gap = dp(6), h = dp(44);
    const float u  = (W - dp(48) - gap * 9.f) / 10.f;
    float       y  = f1.y + dp(28);
    KeyRow("1234567890", x0, y, u, h, gap);                         y += h + gap;
    KeyRow("qwertyuiop", x0, y, u, h, gap);                         y += h + gap;
    KeyRow("asdfghjkl",  x0 + (u + gap) * 0.5f, y, u, h, gap);      y += h + gap;
    const float ws = u * 1.5f + gap * 0.5f;                         // Shift и Backspace — по полторы клавиши
    if (Key("shift", ImRect(ImVec2(x0, y), ImVec2(x0 + ws, y + h)), "", EKey::Shift)) g_bShift = !g_bShift;
    KeyRow("zxcvbnm", x0 + ws + gap, y, u, h, gap);
    if (Key("back", ImRect(ImVec2(W - dp(24) - ws, y), ImVec2(W - dp(24), y + h)), "", EKey::Backspace))
    {
        const size_t n = strlen(g_szText);
        if (n) g_szText[n - 1] = 0;
    }
    y += h + gap;
    KeyRow("-_.", x0, y, u, h, gap);
    const float xs = x0 + (u + gap) * 3.f;
    if (Key("space", ImRect(ImVec2(xs, y), ImVec2(W - dp(24), y + h)), "space", EKey::Space)) Type(' ');

    ImGui::End();
}
