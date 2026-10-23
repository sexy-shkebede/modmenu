#include "ui.hpp"
#include "imgui_internal.h"

#include <ctype.h>
#include <float.h>
#include <initializer_list>
#include <math.h>
#include <stdio.h>
#include <unistd.h>

namespace ui
{
    static ImFont* s_pFont = nullptr;

    static inline float Clamp01(float t) { return t < 0.f ? 0.f : (t > 1.f ? 1.f : t); }

    // ================================================================ цвет
    ImU32 Col(unsigned rgb, float a)
    {
        static float s_aLinear[256];
        static bool  s_bReady = false;
        if (!s_bReady)
        {
            for (int i = 0; i < 256; i++)
            {
                const float c = (float)i / 255.f;
                s_aLinear[i] = c <= 0.04045f ? c / 12.92f : powf((c + 0.055f) / 1.055f, 2.4f);
            }
            s_bReady = true;
        }
        const auto lin = [](unsigned v) { return (ImU32)(s_aLinear[v & 0xFFu] * 255.f + 0.5f); };
        const float alpha = Clamp01(a * ImGui::GetStyle().Alpha);
        return IM_COL32(lin(rgb >> 16), lin(rgb >> 8), lin(rgb), (ImU32)(alpha * 255.f + 0.5f));
    }

    unsigned Mix(unsigned a, unsigned b, float t)
    {
        const auto ch = [&](int shift)
        {
            const float x = (float)((a >> shift) & 0xFFu), y = (float)((b >> shift) & 0xFFu);
            return (unsigned)(x + (y - x) * t + 0.5f) << shift;
        };
        return ch(16) | ch(8) | ch(0);
    }

    // ================================================================ движение
    // Отрезок кубической Безье (x0,y0)…(x3,y3): x(u) монотонна — ищем u по x бисекцией.
    static float CubicSegment(float x0, float y0, float x1, float y1, float x2, float y2, float x3, float y3, float x)
    {
        float lo = 0.f, hi = 1.f, u = 0.5f;
        for (int i = 0; i < 24; i++)
        {
            u = (lo + hi) * 0.5f;
            const float v  = 1.f - u;
            const float xu = v * v * v * x0 + 3.f * v * v * u * x1 + 3.f * v * u * u * x2 + u * u * u * x3;
            (xu < x ? lo : hi) = u;
        }
        const float v = 1.f - u;
        return v * v * v * y0 + 3.f * v * v * u * y1 + 3.f * v * u * u * y2 + u * u * u * y3;
    }

    float Emphasized(float t)
    {
        // M 0,0 C 0.05,0 0.133333,0.06 0.166666,0.4 C 0.208333,0.82 0.25,1 1,1
        t = Clamp01(t);
        return t < 0.166666f ? CubicSegment(0.f, 0.f, 0.05f, 0.f, 0.133333f, 0.06f, 0.166666f, 0.4f, t)
                             : CubicSegment(0.166666f, 0.4f, 0.208333f, 0.82f, 0.25f, 1.f, 1.f, 1.f, t);
    }

    float EmphasizedDecelerate(float t) { return CubicSegment(0.f, 0.f, 0.05f, 0.7f, 0.1f, 1.f, 1.f, 1.f, Clamp01(t)); }

    float AnimDt() { return g_bMotion ? ImGui::GetIO().DeltaTime : 1.f; }

    float Approach(float cur, float target, float speed, float dt) { return cur + (target - cur) * (1.f - expf(-speed * dt)); }
    float MoveTowards(float cur, float target, float maxStep)      { return cur < target ? fminf(cur + maxStep, target) : fmaxf(cur - maxStep, target); }

    // ================================================================ шрифт
    void LoadFont()
    {
        // Roboto — шрифт M3 — есть в прошивке шлема. Если вдруг нет — встроенный масштабируемый
        // шрифт ImGui. Файл проверяем заранее: ImGui на отсутствующем файле ругается.
        static const char* const kPaths[] =
        {
            "/system/fonts/Roboto-Regular.ttf",
            "/system/fonts/RobotoStatic-Regular.ttf",
        };
        ImGuiIO& io = ImGui::GetIO();
        s_pFont = nullptr;
        for (const char* pszPath : kPaths)
            if (access(pszPath, R_OK) == 0 && (s_pFont = io.Fonts->AddFontFromFileTTF(pszPath, type::BodyLarge)) != nullptr)
                break;
        if (!s_pFont)
            s_pFont = io.Fonts->AddFontDefaultVector();
        ImGui::GetStyle().FontSizeBase = type::BodyLarge;
    }

    void WarmupGlyphs()
    {
        // Один раз растеризуем ASCII во всех размерах шкалы — без догрузок текстуры
        // (и микрофризов) при первом открытии страниц.
        static ImFont* s_pWarmed = nullptr;
        if (!s_pFont || s_pWarmed == s_pFont) return;
        s_pWarmed = s_pFont;

        char szAscii[96];
        for (int i = 0; i < 95; i++) szAscii[i] = (char)(32 + i);
        szAscii[95] = 0;
        for (float size : { type::TitleLarge, type::BodyLarge, type::BodyMedium, type::BodySmall })
            s_pFont->CalcTextSizeA(size, FLT_MAX, 0.f, szAscii);
    }

    ImVec2 TextSize(float size, const char* text, float wrapWidth)
    {
        return s_pFont ? s_pFont->CalcTextSizeA(size, FLT_MAX, wrapWidth, text) : ImVec2(0.f, 0.f);
    }

    void Text(ImDrawList* dl, float size, ImVec2 pos, ImU32 col, const char* text, float wrapWidth)
    {
        dl->AddText(s_pFont, size, ImVec2(floorf(pos.x), floorf(pos.y)), col, text, nullptr, wrapWidth);
    }

    // ================================================================ фигуры и иконки
    void RoundRect(ImDrawList* dl, ImVec2 a, ImVec2 b, float rTL, float rTR, float rBR, float rBL, ImU32 col)
    {
        if ((col & IM_COL32_A_MASK) == 0 || b.x - a.x < 0.5f || b.y - a.y < 0.5f) return;
        const float rMax = ImMin(b.x - a.x, b.y - a.y) * 0.5f;
        rTL = ImMin(rTL, rMax); rTR = ImMin(rTR, rMax); rBR = ImMin(rBR, rMax); rBL = ImMin(rBL, rMax);
        dl->PathArcTo(ImVec2(a.x + rTL, a.y + rTL), rTL, IM_PI,        IM_PI * 1.5f);
        dl->PathArcTo(ImVec2(b.x - rTR, a.y + rTR), rTR, IM_PI * 1.5f, IM_PI * 2.f);
        dl->PathArcTo(ImVec2(b.x - rBR, b.y - rBR), rBR, 0.f,          IM_PI * 0.5f);
        dl->PathArcTo(ImVec2(a.x + rBL, b.y - rBL), rBL, IM_PI * 0.5f, IM_PI);
        dl->PathFillConvex(col);
    }

    void Icon(ImDrawList* dl, EIcon icon, ImVec2 c, float s, ImU32 col)
    {
        // Координаты — в сетке 24×24 Material Symbols
        const auto  P      = [&](float x, float y) { return ImVec2(c.x + (x - 12.f) * s / 24.f, c.y + (y - 12.f) * s / 24.f); };
        const float stroke = s / 12.f;   // 2 из 24 — толщина линий Material Symbols
        switch (icon)
        {
        case EIcon::Flight:
        {
            // Обход по часовой стрелке (y вниз): так его ждёт заливка вогнутых полигонов ImGui
            const ImVec2 pts[] = {
                P(13, 13.5f), P(13, 19), P(15, 20.5f), P(15, 22), P(11.5f, 21), P(8, 22), P(8, 20.5f), P(10, 19),
                P(10, 13.5f), P(2, 16), P(2, 14), P(10, 9), P(10, 3.5f), P(10.2f, 2.6f), P(10.8f, 2.1f), P(11.5f, 2.f),
                P(12.2f, 2.1f), P(12.8f, 2.6f), P(13, 3.5f), P(13, 9), P(21, 14), P(21, 16) };
            dl->AddConcavePolyFilled(pts, IM_ARRAYSIZE(pts), col);
            break;
        }
        case EIcon::GridView:
            for (int i = 0; i < 4; i++)
            {
                const float x = (i % 2) ? 13.f : 3.f, y = (i / 2) ? 13.f : 3.f;
                dl->AddRectFilled(P(x, y), P(x + 8.f, y + 8.f), col, s / 16.f);
            }
            break;
        case EIcon::Bolt:
        {
            const ImVec2 pts[] = { P(11, 21), P(10, 21), P(11, 14), P(7.5f, 14), P(13, 3), P(14, 3), P(13, 10), P(16.5f, 10) };
            dl->AddConcavePolyFilled(pts, IM_ARRAYSIZE(pts), col);
            break;
        }
        case EIcon::Cloud:
            dl->AddCircleFilled(P(6.f, 14.f),     6.f * s / 24.f, col, 24);
            dl->AddCircleFilled(P(12.1f, 11.5f), 7.5f * s / 24.f, col, 28);
            dl->AddCircleFilled(P(19.f, 15.f),    5.f * s / 24.f, col, 24);
            dl->AddRectFilled(P(6.f, 14.f), P(19.f, 20.f), col);
            break;
        case EIcon::Settings:   // шестерёнка: кольцо и 8 зубцов
        {
            dl->AddCircle(c, 6.f * s / 24.f, col, 28, 3.4f * s / 24.f);
            const float k = s / 24.f;
            for (int i = 0; i < 8; i++)
            {
                const float  a = (float)i * IM_PI / 4.f;
                const ImVec2 d(cosf(a), sinf(a)), t(-d.y, d.x);
                const auto   Q = [&](float r, float w) { return ImVec2(c.x + (d.x * r + t.x * w) * k, c.y + (d.y * r + t.y * w) * k); };
                dl->AddQuadFilled(Q(6.5f, -1.8f), Q(10.f, -1.5f), Q(10.f, 1.5f), Q(6.5f, 1.8f), col);
            }
            break;
        }
        case EIcon::Info:
            dl->AddCircle(c, 9.f * s / 24.f, col, 32, stroke);
            dl->AddCircleFilled(P(12.f, 7.8f), 1.3f * s / 24.f, col, 12);
            dl->AddRectFilled(P(11.f, 10.5f), P(13.f, 17.f), col);
            break;
        case EIcon::Check:
        {
            const ImVec2 pts[] = { P(5.f, 12.5f), P(9.5f, 17.f), P(19.f, 7.5f) };
            dl->AddPolyline(pts, 3, col, stroke);
            break;
        }
        case EIcon::Edit:
        {
            const ImVec2 body[] = { P(3.f, 17.25f), P(14.06f, 6.19f), P(17.81f, 9.94f), P(6.75f, 21.f), P(3.f, 21.f) };
            const ImVec2 cap[]  = { P(15.2f, 5.05f), P(17.05f, 3.2f), P(20.8f, 6.95f), P(18.95f, 8.8f) };
            dl->AddConcavePolyFilled(body, IM_ARRAYSIZE(body), col);
            dl->AddConcavePolyFilled(cap,  IM_ARRAYSIZE(cap),  col);
            break;
        }
        case EIcon::Add:
            dl->AddLine(P(12, 5), P(12, 19), col, stroke);
            dl->AddLine(P(5, 12), P(19, 12), col, stroke);
            break;
        case EIcon::Backspace:
        {
            const ImVec2 pts[] = { P(8.5f, 5.5f), P(21.f, 5.5f), P(21.f, 18.5f), P(8.5f, 18.5f), P(2.5f, 12.f) };
            dl->AddPolyline(pts, IM_ARRAYSIZE(pts), col, stroke * 0.8f, ImDrawFlags_Closed);
            dl->AddLine(P(11.5f, 9.f), P(17.5f, 15.f), col, stroke * 0.8f);
            dl->AddLine(P(17.5f, 9.f), P(11.5f, 15.f), col, stroke * 0.8f);
            break;
        }
        case EIcon::Shift:
        case EIcon::ShiftOn:
        {
            const ImVec2 pts[] = { P(12.f, 3.5f), P(20.5f, 12.f), P(16.f, 12.f), P(16.f, 19.5f), P(8.f, 19.5f), P(8.f, 12.f), P(3.5f, 12.f) };
            if (icon == EIcon::ShiftOn) dl->AddConcavePolyFilled(pts, IM_ARRAYSIZE(pts), col);
            else                        dl->AddPolyline(pts, IM_ARRAYSIZE(pts), col, stroke * 0.8f, ImDrawFlags_Closed);
            break;
        }
        case EIcon::Close:
            dl->AddLine(P(6, 6), P(18, 18), col, stroke);
            dl->AddLine(P(18, 6), P(6, 18), col, stroke);
            break;
        case EIcon::ChevronRight:
        {
            const ImVec2 pts[] = { P(9.5f, 6), P(15.5f, 12), P(9.5f, 18) };
            dl->AddPolyline(pts, 3, col, stroke);
            break;
        }
        }
    }

    // ================================================================ компоненты
    static void RowShape(ImDrawList* dl, const ImRect& bb, ERow pos, ImU32 col)
    {
        const bool  bTop    = pos == ERow::Single || pos == ERow::First;
        const bool  bBottom = pos == ERow::Single || pos == ERow::Last;
        const float rTop    = bTop    ? shape::Large : shape::ExtraSmall;
        const float rBottom = bBottom ? shape::Large : shape::ExtraSmall;
        RoundRect(dl, bb.Min, bb.Max, rTop, rTop, rBottom, rBottom, col);
    }

    struct SRowAnim { float fHover; float fPress; };
    static SRowAnim RowAnim(ImGuiID id, bool bHovered, bool bHeld)
    {
        ImGuiStorage* pStore = ImGui::GetStateStorage();
        const float   dt     = AnimDt();
        float* pHover = pStore->GetFloatRef(id ^ 0x68A1u, 0.f);
        float* pPress = pStore->GetFloatRef(id ^ 0x9F3Du, 0.f);
        *pHover = Approach(*pHover, bHovered ? 1.f : 0.f, 20.f, dt);
        *pPress = Approach(*pPress, bHeld    ? 1.f : 0.f, 30.f, dt);
        return { *pHover, *pPress };
    }

    static float Layer(const SRowAnim& a) { return ImMax(state::Hover * a.fHover, state::Pressed * a.fPress); }

    // M3 Switch: дорожка 52×32 dp; ручка 16 dp (выкл) → 24 dp (вкл) → 28 dp (нажата);
    // вокруг ручки — слой состояния 40 dp.
    static void DrawSwitch(ImDrawList* dl, ImVec2 o, float on, const SRowAnim& a, unsigned bg)
    {
        const ImVec2 o1(o.x + dp(52), o.y + dp(32));
        const float  cy       = o.y + dp(16);
        const float  hx       = Lerp(o.x + dp(16), o.x + dp(36), on);
        const float  interact = ImMax(a.fHover, a.fPress);

        const float layer = Layer(a);
        if (layer > 0.002f)
            dl->AddCircleFilled(ImVec2(hx, cy), dp(20), Col(Mix(bg, Mix(role::OnSurface, role::Primary, on), layer)), 48);

        dl->AddRectFilled(o, o1, Col(Mix(role::SurfaceContainerHighest, role::Primary, on)), dp(16));
        if (on < 0.999f)
            dl->AddRect(ImVec2(o.x + dp(1), o.y + dp(1)), ImVec2(o1.x - dp(1), o1.y - dp(1)), Col(role::Outline, 1.f - on), dp(15), dp(2));

        const float    r      = Lerp(Lerp(dp(8), dp(12), on), dp(14), a.fPress);
        const unsigned handle = Mix(Mix(role::Outline,   role::OnSurfaceVariant, interact),
                                    Mix(role::OnPrimary, role::PrimaryContainer, interact), on);
        dl->AddCircleFilled(ImVec2(hx, cy), r, Col(handle), 32);
    }

    bool SwitchRow(const char* headline, const char* supporting, bool* v, ERow pos, bool bEnabled)
    {
        ImGuiWindow* win = ImGui::GetCurrentWindow();
        if (win->SkipItems) return false;

        const ImGuiID id = win->GetID(headline);
        const ImVec2  p  = ImGui::GetCursorScreenPos();
        const float   w  = ImGui::GetContentRegionAvail().x;

        // Строка списка M3: поля 16 dp, текст слева, переключатель справа.
        const float  textW = w - dp(16) - dp(16 + 52 + 16);
        const float  headH = type::BodyLarge;
        const float  supH  = supporting ? TextSize(type::BodyMedium, supporting, textW).y : 0.f;
        const float  gap   = supporting ? dp(2) : 0.f;
        const float  h     = ImMax(dp(supporting ? 72.f : 56.f), dp(12) + headH + gap + supH + dp(12));
        const ImRect bb(p, ImVec2(p.x + w, p.y + h));
        ImGui::ItemSize(bb.GetSize());
        if (!ImGui::ItemAdd(bb, id)) return false;

        // Нажатие по всей строке, в момент нажатия курка (в VR рука при отпускании дёргается).
        bool bHovered = false, bHeld = false;
        const bool bPressed = bEnabled && ImGui::ButtonBehavior(bb, id, &bHovered, &bHeld, ImGuiButtonFlags_PressedOnClick);
        if (bPressed) *v = !*v;

        const SRowAnim a   = RowAnim(id, bHovered, bHeld);
        float*         pOn = ImGui::GetStateStorage()->GetFloatRef(id, *v ? 1.f : 0.f);
        *pOn = MoveTowards(*pOn, *v ? 1.f : 0.f, AnimDt() / dur::Short4);

        ImDrawList*    dl        = win->DrawList;
        const unsigned container = Mix(role::SurfaceContainerHigh, role::OnSurface, Layer(a));
        RowShape(dl, bb, pos, Col(container));

        // M3: у недоступной строки контейнер тот же, содержимое — 38 %
        if (!bEnabled) ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.38f);
        const float cy = (bb.Min.y + bb.Max.y) * 0.5f;
        const float ty = cy - (headH + gap + supH) * 0.5f;
        Text(dl, type::BodyLarge, ImVec2(bb.Min.x + dp(16), ty), Col(role::OnSurface), headline);
        if (supporting)
            Text(dl, type::BodyMedium, ImVec2(bb.Min.x + dp(16), ty + headH + gap), Col(role::OnSurfaceVariant), supporting, textW);

        DrawSwitch(dl, ImVec2(bb.Max.x - dp(16 + 52), cy - dp(16)), Emphasized(*pOn), a, container);
        if (!bEnabled) ImGui::PopStyleVar();
        return bPressed;
    }

    bool SliderRow(const char* headline, float* v, float vMin, float vMax, const char* fmt, ERow pos)
    {
        ImGuiWindow* win = ImGui::GetCurrentWindow();
        if (win->SkipItems) return false;

        const ImGuiID id = win->GetID(headline);
        const ImVec2  p  = ImGui::GetCursorScreenPos();
        const float   w  = ImGui::GetContentRegionAvail().x;
        const ImRect  bb(p, ImVec2(p.x + w, p.y + dp(12) + type::BodyLarge + dp(4) + dp(44) + dp(8)));

        // M3 Slider (2024): дорожка 16 dp, ручка — вертикальная полоска 4×44 dp с зазором 6 dp.
        const float  x0 = bb.Min.x + dp(16), x1 = bb.Max.x - dp(16);
        const float  cy = bb.Max.y - dp(8) - dp(22);
        const ImRect hit(ImVec2(x0 - dp(8), cy - dp(22)), ImVec2(x1 + dp(8), cy + dp(22)));
        ImGui::ItemSize(bb.GetSize());
        if (!ImGui::ItemAdd(hit, id)) return false;

        bool bHovered = false, bHeld = false;
        ImGui::ButtonBehavior(hit, id, &bHovered, &bHeld, ImGuiButtonFlags_PressedOnClick);
        bool bChanged = false;
        const ImGuiIO& io = ImGui::GetIO();
        if (bHeld && ImGui::IsMousePosValid(&io.MousePos))
        {
            const float fNew = vMin + ImClamp((io.MousePos.x - x0) / (x1 - x0), 0.f, 1.f) * (vMax - vMin);
            bChanged = fNew != *v;
            *v = fNew;
        }

        const SRowAnim a  = RowAnim(id, bHovered, bHeld);
        ImDrawList*    dl = win->DrawList;
        RowShape(dl, bb, pos, Col(Mix(role::SurfaceContainerHigh, role::OnSurface, state::Hover * a.fHover)));

        char szValue[32];
        snprintf(szValue, sizeof(szValue), fmt, *v);
        const ImVec2 vs = TextSize(type::BodyMedium, szValue);
        const float  hy = bb.Min.y + dp(12);
        Text(dl, type::BodyLarge,  ImVec2(bb.Min.x + dp(16), hy), Col(role::OnSurface), headline);
        Text(dl, type::BodyMedium, ImVec2(bb.Max.x - dp(16) - vs.x, hy + (type::BodyLarge - type::BodyMedium) * 0.5f), Col(role::OnSurfaceVariant, 1.f - a.fPress), szValue);

        const float t   = ImClamp((*v - vMin) / (vMax - vMin), 0.f, 1.f);
        const float hw  = Lerp(dp(4), dp(2), a.fPress);            // при нажатии ручка сужается
        const float hx  = Lerp(x0 + hw * 0.5f, x1 - hw * 0.5f, t);
        const float ty0 = cy - dp(8), ty1 = cy + dp(8);
        const float rOut = dp(8), rIn = dp(2), gap = dp(6);

        const float aEnd = hx - hw * 0.5f - gap;                    // активная часть — primary
        if (aEnd > x0 + dp(1))
            RoundRect(dl, ImVec2(x0, ty0), ImVec2(aEnd, ty1), rOut, rIn, rIn, rOut, Col(role::Primary));
        const float iStart = hx + hw * 0.5f + gap;                  // неактивная — secondary container
        if (iStart < x1 - dp(1))
        {
            RoundRect(dl, ImVec2(iStart, ty0), ImVec2(x1, ty1), rIn, rOut, rOut, rIn, Col(role::SecondaryContainer));
            if (iStart < x1 - dp(12))                               // точка-ограничитель в конце дорожки
                dl->AddCircleFilled(ImVec2(x1 - dp(8), cy), dp(2), Col(role::Primary), 12);
        }
        const float rh = hw * 0.5f;
        RoundRect(dl, ImVec2(hx - rh, cy - dp(22)), ImVec2(hx + rh, cy + dp(22)), rh, rh, rh, rh, Col(role::Primary));

        // Индикатор значения, пока ручку держат: inverse surface, label large
        if (a.fPress > 0.01f)
        {
            ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * a.fPress);
            const ImVec2 ls = TextSize(type::LabelLarge, szValue);
            const float  bw = ImMax(dp(48), ls.x + dp(32)), bh = dp(44);
            const ImVec2 b0(hx - bw * 0.5f, cy - dp(22) - dp(4) - bh), b1(b0.x + bw, b0.y + bh);
            RoundRect(dl, b0, b1, bh * 0.5f, bh * 0.5f, bh * 0.5f, bh * 0.5f, Col(role::InverseSurface));
            Text(dl, type::LabelLarge, ImVec2(b0.x + (bw - ls.x) * 0.5f, b0.y + (bh - type::LabelLarge) * 0.5f), Col(role::InverseOnSurface), szValue);
            ImGui::PopStyleVar();
        }
        return bChanged;
    }

    void EmptyState(EIcon icon, const char* headline, const char* supporting)
    {
        ImGuiWindow* win = ImGui::GetCurrentWindow();
        if (win->SkipItems) return;

        const ImVec2 p     = ImGui::GetCursorScreenPos();
        const ImVec2 avail = ImGui::GetContentRegionAvail();
        ImGui::Dummy(avail);

        const float  wrap  = ImMin(avail.x, dp(320));
        const ImVec2 hs    = TextSize(type::BodyLarge,  headline,   wrap);
        const ImVec2 ss    = TextSize(type::BodyMedium, supporting, wrap);
        const float  cx    = p.x + avail.x * 0.5f;
        float        y     = p.y + (avail.y - (dp(48) + dp(16) + hs.y + dp(4) + ss.y)) * 0.5f;

        ImDrawList* dl = win->DrawList;
        Icon(dl, icon, ImVec2(cx, y + dp(24)), dp(48), Col(role::OnSurfaceVariant));
        y += dp(48) + dp(16);
        Text(dl, type::BodyLarge, ImVec2(cx - hs.x * 0.5f, y), Col(role::OnSurface), headline, wrap);
        y += hs.y + dp(4);
        Text(dl, type::BodyMedium, ImVec2(cx - ss.x * 0.5f, y), Col(role::OnSurfaceVariant), supporting, wrap);
    }

    void Gap(float dpValue) { ImGui::Dummy(ImVec2(0.f, dp(dpValue))); }

    void Section(const char* title)
    {
        ImGuiWindow* win = ImGui::GetCurrentWindow();
        if (win->SkipItems) return;
        const ImVec2 p = ImGui::GetCursorScreenPos();
        ImGui::Dummy(ImVec2(ImGui::GetContentRegionAvail().x, dp(40)));
        Text(win->DrawList, type::LabelLarge, ImVec2(p.x + dp(16), p.y + dp(40) - dp(10) - type::LabelLarge), Col(role::Primary), title);
    }

    bool SegmentedRow(const char* headline, const char* const* items, int count, int* v, ERow pos)
    {
        ImGuiWindow* win = ImGui::GetCurrentWindow();
        if (win->SkipItems) return false;

        const ImGuiID id = win->GetID(headline);
        const ImVec2  p  = ImGui::GetCursorScreenPos();
        const float   w  = ImGui::GetContentRegionAvail().x;
        const ImRect  bb(p, ImVec2(p.x + w, p.y + dp(12) + type::BodyLarge + dp(8) + dp(40) + dp(12)));
        ImGui::ItemSize(bb.GetSize());
        if (!ImGui::IsRectVisible(bb.Min, bb.Max)) return false;

        ImDrawList*    dl        = win->DrawList;
        const unsigned container = role::SurfaceContainerHigh;
        RowShape(dl, bb, pos, Col(container));
        Text(dl, type::BodyLarge, ImVec2(bb.Min.x + dp(16), bb.Min.y + dp(12)), Col(role::OnSurface), headline);

        const float x0 = bb.Min.x + dp(16), x1 = bb.Max.x - dp(16);
        const float y1 = bb.Max.y - dp(12), y0 = y1 - dp(40);
        const float sw = (x1 - x0) / (float)count, r = dp(20);
        bool bChanged = false;
        for (int i = 0; i < count; i++)
        {
            const ImGuiID sid = id + 1u + (ImGuiID)i;
            const ImRect  sb(ImVec2(x0 + sw * (float)i, y0), ImVec2(x0 + sw * (float)(i + 1), y1));
            ImGui::ItemAdd(sb, sid);
            bool bHovered = false, bHeld = false;
            if (ImGui::ButtonBehavior(sb, sid, &bHovered, &bHeld, ImGuiButtonFlags_PressedOnClick) && *v != i)
            {
                *v = i;
                bChanged = true;
            }
            const SRowAnim a    = RowAnim(sid, bHovered, bHeld);
            float*         pSel = ImGui::GetStateStorage()->GetFloatRef(sid ^ 0x51E7u, *v == i ? 1.f : 0.f);
            *pSel = MoveTowards(*pSel, *v == i ? 1.f : 0.f, AnimDt() / dur::Short4);
            const float    sel  = *pSel;

            const unsigned on   = Mix(role::OnSurface, role::OnSecondaryContainer, sel);
            const unsigned fill = Mix(Mix(container, role::SecondaryContainer, sel), on, Layer(a));
            const float    rl   = i == 0 ? r : 0.f, rr = i == count - 1 ? r : 0.f;
            if (fill != container)
                RoundRect(dl, sb.Min, sb.Max, rl, rr, rr, rl, Col(fill));

            // Галочка "вырастает" у выбранного сегмента, подпись сдвигается
            const ImVec2 ls    = TextSize(type::LabelLarge, items[i]);
            const float  iconW = dp(18) * sel, gapW = dp(8) * sel;
            const float  cx    = (sb.Min.x + sb.Max.x) * 0.5f - (iconW + gapW + ls.x) * 0.5f;
            const float  cy    = (y0 + y1) * 0.5f;
            if (sel > 0.01f)
                Icon(dl, EIcon::Check, ImVec2(cx + iconW * 0.5f, cy), dp(18) * sel, Col(on));
            Text(dl, type::LabelLarge, ImVec2(cx + iconW + gapW, cy - type::LabelLarge * 0.5f), Col(on), items[i]);
        }
        dl->AddRect(ImVec2(x0 + dp(0.5f), y0 + dp(0.5f)), ImVec2(x1 - dp(0.5f), y1 - dp(0.5f)), Col(role::Outline), r, dp(1));
        for (int i = 1; i < count; i++)
            dl->AddLine(ImVec2(x0 + sw * (float)i, y0), ImVec2(x0 + sw * (float)i, y1), Col(role::Outline), dp(1));
        return bChanged;
    }

    static void Swatch(ImDrawList* dl, ImVec2 c, float r, const unsigned* rgb)
    {
        dl->PathArcTo(c, r, IM_PI, IM_PI * 2.f);                        dl->PathFillConvex(Col(rgb[0]));   // верх — primary
        dl->PathLineTo(c); dl->PathArcTo(c, r, IM_PI * 0.5f, IM_PI);    dl->PathFillConvex(Col(rgb[1]));   // низ слева — secondary
        dl->PathLineTo(c); dl->PathArcTo(c, r, 0.f, IM_PI * 0.5f);      dl->PathFillConvex(Col(rgb[2]));   // низ справа — tertiary
    }

    bool ColorGridRow(const char* headline, const char* trailing, const unsigned (*colors)[3], int count, int* v, ERow pos)
    {
        ImGuiWindow* win = ImGui::GetCurrentWindow();
        if (win->SkipItems) return false;

        const ImGuiID id    = win->GetID(headline);
        const ImVec2  p     = ImGui::GetCursorScreenPos();
        const float   w     = ImGui::GetContentRegionAvail().x;
        const float   D     = dp(40), gap = dp(12);
        const int     cols  = ImMax(1, (int)((w - dp(32) + gap) / (D + gap)));
        const int     rows  = (count + cols - 1) / cols;
        const float   gridH = (float)rows * D + (float)(rows - 1) * gap;
        const ImRect  bb(p, ImVec2(p.x + w, p.y + dp(12) + type::BodyLarge + dp(12) + gridH + dp(16)));
        ImGui::ItemSize(bb.GetSize());
        if (!ImGui::IsRectVisible(bb.Min, bb.Max)) return false;

        ImDrawList*    dl        = win->DrawList;
        const unsigned container = role::SurfaceContainerHigh;
        RowShape(dl, bb, pos, Col(container));
        Text(dl, type::BodyLarge, ImVec2(bb.Min.x + dp(16), bb.Min.y + dp(12)), Col(role::OnSurface), headline);
        if (trailing)
        {
            const ImVec2 ts = TextSize(type::BodyMedium, trailing);
            Text(dl, type::BodyMedium, ImVec2(bb.Max.x - dp(16) - ts.x, bb.Min.y + dp(12) + (type::BodyLarge - type::BodyMedium) * 0.5f), Col(role::OnSurfaceVariant), trailing);
        }

        const float gx = bb.Min.x + dp(16), gy = bb.Min.y + dp(12) + type::BodyLarge + dp(12);
        bool bChanged = false;
        for (int i = 0; i < count; i++)
        {
            const ImGuiID sid = id + 1u + (ImGuiID)i;
            const ImVec2  c(gx + (float)(i % cols) * (D + gap) + D * 0.5f, gy + (float)(i / cols) * (D + gap) + D * 0.5f);
            const ImRect  sb(ImVec2(c.x - D * 0.5f - dp(4), c.y - D * 0.5f - dp(4)), ImVec2(c.x + D * 0.5f + dp(4), c.y + D * 0.5f + dp(4)));
            ImGui::ItemAdd(sb, sid);
            bool bHovered = false, bHeld = false;
            if (ImGui::ButtonBehavior(sb, sid, &bHovered, &bHeld, ImGuiButtonFlags_PressedOnClick) && *v != i)
            {
                *v = i;
                bChanged = true;
            }
            const SRowAnim a    = RowAnim(sid, bHovered, bHeld);
            float*         pSel = ImGui::GetStateStorage()->GetFloatRef(sid ^ 0x51E7u, *v == i ? 1.f : 0.f);
            *pSel = MoveTowards(*pSel, *v == i ? 1.f : 0.f, AnimDt() / dur::Short4);

            const float layer = Layer(a);
            if (layer > 0.002f)
                dl->AddCircleFilled(c, D * 0.5f + dp(6), Col(Mix(container, role::OnSurface, layer)), 40);
            // Выбранный кружок чуть сжимается, вокруг появляется кольцо
            if (*pSel > 0.01f)
                dl->AddCircle(c, D * 0.5f + dp(3), Col(role::OnSurface, *pSel), 40, dp(2));
            Swatch(dl, c, D * 0.5f * Lerp(1.f, 0.78f, *pSel), colors[i]);
        }
        return bChanged;
    }

    void PersonRow(const char* name, const char* supporting, ERow pos)
    {
        ImGuiWindow* win = ImGui::GetCurrentWindow();
        if (win->SkipItems) return;

        const ImVec2 p = ImGui::GetCursorScreenPos();
        const float  w = ImGui::GetContentRegionAvail().x;
        const ImRect bb(p, ImVec2(p.x + w, p.y + dp(72)));
        ImGui::Dummy(bb.GetSize());
        if (!ImGui::IsItemVisible()) return;

        ImDrawList* dl = win->DrawList;
        RowShape(dl, bb, pos, Col(role::SurfaceContainerHigh));

        // Аватар M3: круг 40 dp primary container с инициалом
        const float  cy = (bb.Min.y + bb.Max.y) * 0.5f;
        const ImVec2 ac(bb.Min.x + dp(16 + 20), cy);
        const char   szInitial[2] = { (char)toupper((unsigned char)name[0]), 0 };
        const ImVec2 is = TextSize(type::BodyLarge, szInitial);
        dl->AddCircleFilled(ac, dp(20), Col(role::PrimaryContainer), 40);
        Text(dl, type::BodyLarge, ImVec2(ac.x - is.x * 0.5f, ac.y - type::BodyLarge * 0.5f), Col(role::OnPrimaryContainer), szInitial);

        const float ty = cy - (type::BodyLarge + type::BodyMedium) * 0.5f;
        Text(dl, type::BodyLarge,  ImVec2(bb.Min.x + dp(72), ty), Col(role::OnSurface), name);
        Text(dl, type::BodyMedium, ImVec2(bb.Min.x + dp(72), ty + type::BodyLarge), Col(role::OnSurfaceVariant), supporting);
    }

    // ================================================================ кнопки и служебные строки
    static void DrawButton(ImDrawList* dl, const ImRect& bb, const char* label, EButton style, bool bEnabled, const SRowAnim& a)
    {
        const float  r          = bb.GetHeight() * 0.5f;
        const bool   bContainer = style == EButton::Filled || style == EButton::Tonal;
        const ImVec2 ts         = TextSize(type::LabelLarge, label);
        const ImVec2 tp(bb.Min.x + (bb.GetWidth() - ts.x) * 0.5f, bb.Min.y + (bb.GetHeight() - type::LabelLarge) * 0.5f);
        const ImVec2 o0(bb.Min.x + dp(0.5f), bb.Min.y + dp(0.5f)), o1(bb.Max.x - dp(0.5f), bb.Max.y - dp(0.5f));
        if (!bEnabled)
        {
            // M3: недоступная кнопка — контейнер on surface 12 %, подпись 38 %
            if (bContainer)                 dl->AddRectFilled(bb.Min, bb.Max, Col(role::OnSurface, 0.12f), r);
            if (style == EButton::Outlined) dl->AddRect(o0, o1, Col(role::OnSurface, 0.12f), r, dp(1));
            Text(dl, type::LabelLarge, tp, Col(role::OnSurface, 0.38f), label);
            return;
        }
        const unsigned container = style == EButton::Filled ? role::Primary : role::SecondaryContainer;
        const unsigned content   = style == EButton::Filled ? role::OnPrimary
                                 : style == EButton::Tonal  ? role::OnSecondaryContainer : role::Primary;
        const float    layer     = Layer(a);
        if (bContainer)
            dl->AddRectFilled(bb.Min, bb.Max, Col(Mix(container, content, layer)), r);
        else if (layer > 0.002f)   // без контейнера слой полупрозрачный; ×0.35 — поправка на линейное смешивание
            dl->AddRectFilled(bb.Min, bb.Max, Col(content, layer * 0.35f), r);
        if (style == EButton::Outlined)
            dl->AddRect(o0, o1, Col(role::Outline), r, dp(1));
        Text(dl, type::LabelLarge, tp, Col(content), label);
    }

    static bool SubButton(ImGuiID id, const ImRect& bb, const char* label, EButton style, bool bEnabled)
    {
        ImGui::ItemAdd(bb, id);
        bool bHovered = false, bHeld = false;
        const bool bPressed = bEnabled && ImGui::ButtonBehavior(bb, id, &bHovered, &bHeld, ImGuiButtonFlags_PressedOnClick);
        DrawButton(ImGui::GetWindowDrawList(), bb, label, style, bEnabled, RowAnim(id, bHovered, bHeld));
        return bPressed;
    }

    bool Button(const char* label, EButton style, float width, bool bEnabled)
    {
        ImGuiWindow* win = ImGui::GetCurrentWindow();
        if (win->SkipItems) return false;
        const ImVec2 p = ImGui::GetCursorScreenPos();
        const ImRect bb(p, ImVec2(p.x + (width > 0.f ? width : TextSize(type::LabelLarge, label).x + dp(48)), p.y + dp(40)));
        ImGui::ItemSize(bb.GetSize());
        return SubButton(win->GetID(label), bb, label, style, bEnabled);
    }

    int ActionsRow(const char* labelA, const char* labelB, bool bEnabledA, bool bEnabledB, ERow pos)
    {
        ImGuiWindow* win = ImGui::GetCurrentWindow();
        if (win->SkipItems) return 0;
        const ImVec2 p = ImGui::GetCursorScreenPos();
        const float  w = ImGui::GetContentRegionAvail().x;
        const ImRect bb(p, ImVec2(p.x + w, p.y + dp(72)));
        ImGui::ItemSize(bb.GetSize());
        if (!ImGui::IsRectVisible(bb.Min, bb.Max)) return 0;

        RowShape(win->DrawList, bb, pos, Col(role::SurfaceContainerHigh));
        const float  bw = (w - dp(32 + 8)) * 0.5f, y = bb.Min.y + dp(16);
        const ImRect a(ImVec2(bb.Min.x + dp(16), y), ImVec2(bb.Min.x + dp(16) + bw, y + dp(40)));
        const ImRect b(ImVec2(a.Max.x + dp(8), y),   ImVec2(a.Max.x + dp(8) + bw, y + dp(40)));
        int iResult = 0;
        if (SubButton(win->GetID(labelA), a, labelA, EButton::Tonal, bEnabledA)) iResult = 1;
        if (SubButton(win->GetID(labelB), b, labelB, EButton::Tonal, bEnabledB)) iResult = 2;
        return iResult;
    }

    bool ValueRow(const char* headline, const char* value, ERow pos)
    {
        ImGuiWindow* win = ImGui::GetCurrentWindow();
        if (win->SkipItems) return false;
        const ImGuiID id = win->GetID(headline);
        const ImVec2  p  = ImGui::GetCursorScreenPos();
        const ImRect  bb(p, ImVec2(p.x + ImGui::GetContentRegionAvail().x, p.y + dp(72)));
        ImGui::ItemSize(bb.GetSize());
        if (!ImGui::ItemAdd(bb, id)) return false;

        bool bHovered = false, bHeld = false;
        const bool     bPressed = ImGui::ButtonBehavior(bb, id, &bHovered, &bHeld, ImGuiButtonFlags_PressedOnClick);
        const SRowAnim a        = RowAnim(id, bHovered, bHeld);
        ImDrawList*    dl       = win->DrawList;
        RowShape(dl, bb, pos, Col(Mix(role::SurfaceContainerHigh, role::OnSurface, Layer(a))));

        const float cy = (bb.Min.y + bb.Max.y) * 0.5f;
        const float ty = cy - (type::BodyLarge + type::BodyMedium) * 0.5f;
        dl->PushClipRect(bb.Min, ImVec2(bb.Max.x - dp(56), bb.Max.y), true);
        Text(dl, type::BodyLarge,  ImVec2(bb.Min.x + dp(16), ty), Col(role::OnSurface), headline);
        Text(dl, type::BodyMedium, ImVec2(bb.Min.x + dp(16), ty + type::BodyLarge), Col(role::OnSurfaceVariant), value);
        dl->PopClipRect();
        Icon(dl, EIcon::Edit, ImVec2(bb.Max.x - dp(16 + 12), cy), dp(24), Col(role::OnSurfaceVariant));
        return bPressed;
    }

    void StatGrid(const SStat* stats, int count)
    {
        ImGuiWindow* win = ImGui::GetCurrentWindow();
        if (win->SkipItems || count <= 0) return;
        const ImVec2 p    = ImGui::GetCursorScreenPos();
        const float  w    = ImGui::GetContentRegionAvail().x, gap = dp(2), th = dp(72);
        const int    rows = (count + 1) / 2;
        ImGui::Dummy(ImVec2(w, (float)rows * th + (float)(rows - 1) * gap));
        if (!ImGui::IsItemVisible()) return;

        ImDrawList* dl = win->DrawList;
        const float tw = (w - gap) * 0.5f, L = shape::Large, S = shape::ExtraSmall;
        for (int i = 0; i < count; i++)
        {
            const int    col = i % 2, row = i / 2;
            const bool   bTop = row == 0, bBottom = row == rows - 1;
            const ImVec2 a(p.x + (float)col * (tw + gap), p.y + (float)row * (th + gap)), b(a.x + tw, a.y + th);
            RoundRect(dl, a, b, bTop && col == 0 ? L : S, bTop && col == 1 ? L : S,
                                bBottom && col == 1 ? L : S, bBottom && col == 0 ? L : S, Col(role::SurfaceContainerHigh));
            dl->PushClipRect(a, ImVec2(b.x - dp(8), b.y), true);
            Text(dl, type::BodyMedium, ImVec2(a.x + dp(16), a.y + dp(12)), Col(role::OnSurfaceVariant), stats[i].label);
            Text(dl, type::TitleLarge, ImVec2(a.x + dp(16), a.y + dp(12) + type::BodyMedium), Col(role::OnSurface), stats[i].value);
            dl->PopClipRect();
        }
    }

    void SupportText(const char* text)
    {
        ImGuiWindow* win = ImGui::GetCurrentWindow();
        if (win->SkipItems) return;
        const ImVec2 p    = ImGui::GetCursorScreenPos();
        const float  w    = ImGui::GetContentRegionAvail().x;
        const float  wrap = w - dp(32);
        ImGui::Dummy(ImVec2(w, TextSize(type::BodyMedium, text, wrap).y + dp(16)));
        Text(win->DrawList, type::BodyMedium, ImVec2(p.x + dp(16), p.y + dp(8)), Col(role::OnSurfaceVariant), text, wrap);
    }

    void Snackbar(const char* text, float alpha)
    {
        if (alpha <= 0.01f || !text || !*text) return;
        ImGuiWindow* win  = ImGui::GetCurrentWindow();
        const float  wrap = win->Size.x - dp(32);
        const ImVec2 ts   = TextSize(type::BodyMedium, text, wrap);
        const float  h    = ImMax(dp(48), ts.y + dp(28));
        const float  y1   = win->Pos.y + win->Size.y - dp(8) + dp(16) * (1.f - alpha);
        const ImVec2 b0(win->Pos.x, y1 - h), b1(win->Pos.x + win->Size.x, y1);

        // M3 snackbar: inverse surface, форма extra small, текст body medium
        ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * alpha);
        const float r = shape::ExtraSmall;
        RoundRect(win->DrawList, b0, b1, r, r, r, r, Col(role::InverseSurface));
        Text(win->DrawList, type::BodyMedium, ImVec2(b0.x + dp(16), b0.y + (h - ts.y) * 0.5f), Col(role::InverseOnSurface), text, wrap);
        ImGui::PopStyleVar();
    }
}
