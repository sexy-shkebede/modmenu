#pragma once

#include "imgui.h"

// Material Design 3 для VR-панели: цветовые роли, типографика, формы, слои состояний,
// кривые движения и компоненты (сегментированный список, переключатель, слайдер).
namespace ui
{
    // ---------- Плотность ----------
    // Панель 1440×1080 px = 720×540 dp (medium window class), 1 dp = 2 px текстуры.
    inline constexpr float kDp = 2.0f;
    constexpr float dp(float v) { return v * kDp; }
    // sp → размер шрифта ImGui (ImGui задаёт высоту строки шрифта, у Roboto это ≈ 1.17 em).
    constexpr float sp(float v) { return v * kDp * 1.17f; }

    // ---------- Цветовые роли M3 ----------
    // Значения по умолчанию — базовая тёмная схема (seed #6750A4). Во время работы роли
    // пересчитывает theme::Apply (динамический цвет M3 из выбранного исходного цвета).
#define UI_COLOR_ROLES(X) \
    X(Primary,                 0xD0BCFF) \
    X(OnPrimary,               0x381E72) \
    X(PrimaryContainer,        0x4F378B) \
    X(OnPrimaryContainer,      0xEADDFF) \
    X(Secondary,               0xCCC2DC) \
    X(OnSecondary,             0x332D41) \
    X(SecondaryContainer,      0x4A4458) \
    X(OnSecondaryContainer,    0xE8DEF8) \
    X(Tertiary,                0xEFB8C8) \
    X(OnTertiary,              0x492532) \
    X(TertiaryContainer,       0x633B48) \
    X(OnTertiaryContainer,     0xFFD8E4) \
    X(Error,                   0xF2B8B5) \
    X(OnError,                 0x601410) \
    X(Surface,                 0x141218) \
    X(SurfaceContainerLowest,  0x0F0D13) \
    X(SurfaceContainerLow,     0x1D1B20) \
    X(SurfaceContainer,        0x211F26) \
    X(SurfaceContainerHigh,    0x2B2930) \
    X(SurfaceContainerHighest, 0x36343B) \
    X(OnSurface,               0xE6E0E9) \
    X(OnSurfaceVariant,        0xCAC4D0) \
    X(Outline,                 0x938F99) \
    X(OutlineVariant,          0x49454F) \
    X(InverseSurface,          0xE6E0E9) \
    X(InverseOnSurface,        0x322F35) \

    namespace role
    {
    #define UI_DECLARE_ROLE(name, rgb) inline unsigned name = rgb;
        UI_COLOR_ROLES(UI_DECLARE_ROLE)
    #undef UI_DECLARE_ROLE
    }

    // ---------- Типографика M3 (Roboto) ----------
    namespace type
    {
        inline constexpr float TitleLarge = sp(22);
        inline constexpr float BodyLarge  = sp(16);
        inline constexpr float BodyMedium = sp(14);
        inline constexpr float LabelLarge = sp(14);
        inline constexpr float BodySmall  = sp(12);
    }

    // ---------- Формы M3 (theme::Apply() масштабирует их настройкой Corners) ----------
    namespace shape
    {
        inline float ExtraSmall = dp(4);
        inline float Small      = dp(8);
        inline float Large      = dp(16);
        inline float ExtraLarge = dp(28);
    }

    // ---------- Слои состояний M3: непрозрачность "on"-цвета поверх контейнера ----------
    namespace state
    {
        inline constexpr float Hover   = 0.08f;
        inline constexpr float Pressed = 0.10f;
    }

    // ---------- Движение M3 ----------
    float Emphasized          (float t);   // путь emphasized из Android (две кубические кривые)
    float EmphasizedDecelerate(float t);   // cubic-bezier(0.05, 0.7, 0.1, 1)
    // Длительность с учётом настройки Animations (выключено — переходы мгновенные).
    inline float g_fMotionScale = 1.f;
    inline float Dur(float seconds) { float s = seconds * g_fMotionScale; return s > 1e-4f ? s : 1e-4f; }
    namespace dur
    {
        inline constexpr float Short4  = 0.20f;
        inline constexpr float Medium2 = 0.30f;
        inline constexpr float Long2   = 0.50f;
    }

    // ---------- Цвет ----------
    // Цвета — обычные sRGB 0xRRGGBB. Col() переводит в линейный (текстура меню хранится в sRGB)
    // и умножает альфу на общую прозрачность панели. Mix() смешивает в sRGB — так слои
    // состояний выглядят как на Android, а не пересвечены линейным смешиванием.
    ImU32    Col(unsigned rgb, float a = 1.f);
    unsigned Mix(unsigned rgbA, unsigned rgbB, float t);

    // ---------- Анимация ----------
    inline bool g_bMotion = true;   // настройка Animations: выключено — переходы мгновенные
    float AnimDt();                 // шаг анимаций за кадр (1, если анимации выключены)
    float Approach   (float cur, float target, float speed, float dt);
    float MoveTowards(float cur, float target, float maxStep);
    inline float Lerp(float a, float b, float t) { return a + (b - a) * t; }

    // ---------- Шрифт и текст ----------
    void   LoadFont();
    void   WarmupGlyphs();
    ImVec2 TextSize(float size, const char* text, float wrapWidth = 0.f);
    void   Text(ImDrawList* dl, float size, ImVec2 pos, ImU32 col, const char* text, float wrapWidth = 0.f);

    // ---------- Фигуры и иконки ----------
    void RoundRect(ImDrawList* dl, ImVec2 a, ImVec2 b, float rTL, float rTR, float rBR, float rBL, ImU32 col);

    enum class EIcon { Flight, GridView, Bolt, Cloud, Settings, Info, Close, ChevronRight, Check, Edit, Add, Backspace, Shift, ShiftOn };
    void Icon(ImDrawList* dl, EIcon icon, ImVec2 center, float size, ImU32 col);   // Material Symbols, сетка 24

    // ---------- Компоненты для страниц ----------
    // Строки собираются в группы, как список в настройках Android: внешние углы группы
    // скруглены сильно (large), внутренние — слегка (extra small).
    enum class ERow { Single, First, Middle, Last };

    bool SwitchRow (const char* headline, const char* supporting, bool* v, ERow pos = ERow::Single, bool bEnabled = true);
    bool SliderRow (const char* headline, float* v, float vMin, float vMax, const char* fmt, ERow pos = ERow::Single);
    void EmptyState(EIcon icon, const char* headline, const char* supporting);

    // Подзаголовок списка (label large, primary)
    void Section(const char* title);
    // Сегментированная кнопка M3 (40 dp, контур outline, выбранный — secondary container с галочкой)
    bool SegmentedRow(const char* headline, const char* const* items, int count, int* v, ERow pos = ERow::Single);
    // Сетка кружков-превью цветовых схем (primary / secondary / tertiary), как выбор темы в Android
    bool ColorGridRow(const char* headline, const char* trailing, const unsigned (*colors)[3], int count, int* v, ERow pos = ERow::Single);
    // Строка с аватаром-инициалом: имя + подпись
    void PersonRow(const char* name, const char* supporting, ERow pos = ERow::Single);

    // Кнопки M3: высота 40 dp, форма full. width 0 — по подписи
    enum class EButton { Filled, Tonal, Outlined, Text };
    bool Button(const char* label, EButton style, float width = 0.f, bool bEnabled = true);
    // Плитка с двумя tonal-кнопками рядом; возвращает 1 или 2 — какую нажали, иначе 0
    int  ActionsRow(const char* labelA, const char* labelB, bool bEnabledA, bool bEnabledB, ERow pos = ERow::Single);
    // Строка "заголовок + значение" с карандашом справа; true — нажали (открыть редактирование)
    bool ValueRow(const char* headline, const char* value, ERow pos = ERow::Single);
    // Сетка плиток статистики в две колонки: подпись и крупное значение
    struct SStat { const char* label; const char* value; };
    void StatGrid(const SStat* stats, int count);
    // Поясняющий текст под группой (body medium, on surface variant)
    void SupportText(const char* text);
    // Snackbar M3 внизу текущего окна; alpha — для появления и исчезания
    void Snackbar(const char* text, float alpha);
    void Gap       (float dpValue);   // отступ между группами
}
