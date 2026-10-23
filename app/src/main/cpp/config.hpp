#pragma once

// ============================================================================
//  shkbd internal — настройки. Всё, что обычно хочется подкрутить, — здесь.
// ============================================================================

namespace cfg
{
    // Кнопка на ЛЕВОМ контроллере, которая открывает/закрывает меню.
    //   X / Y      — верхние кнопки левого контроллера
    //   StickClick — нажатие на левый стик
    //   Menu       — кнопка ≡ (многие игры вешают на неё свою паузу)
    // Хук видит только те кнопки, которые игра читает сама. Если в какой-то игре
    // меню не открывается — попробуй другую кнопку.
    enum class EToggleButton { X, Y, StickClick, Menu };
    inline constexpr EToggleButton kToggleButton = EToggleButton::Y;

    // Физический размер панели (метры) и разрешение её текстуры. Пропорции держи одинаковыми.
    inline constexpr float kPanelWidthM  = 0.40f;
    inline constexpr float kPanelHeightM = 0.30f;
    inline constexpr int   kPanelResX    = 1440;
    inline constexpr int   kPanelResY    = 1080;

    // Луч спавна. Его картинки (спрайты) рисуются в полосе под панелью в той же текстуре:
    // x, y, ширина, высота в пикселях. Слои OpenXR ссылаются на эти участки.
    // Между участками — пустые поля, чтобы фильтрация текстуры не подмешивала соседей.
    inline constexpr int   kSpriteStripH     = 144;
    inline constexpr int   kBeamSprite[4]    = { 0,   kPanelResY + 8, 32,  128 };
    inline constexpr int   kReticleSprite[4] = { 48,  kPanelResY + 8, 128, 128 };
    inline constexpr int   kLabelSprite[4]   = { 192, kPanelResY + 8, 768, 96  };
    inline constexpr float kSpawnBeamMaxM    = 30.f;    // дальность луча, м
    inline constexpr float kSpawnBeamWidthM  = 0.008f;  // толщина луча, м

    // Слой мира с блоками: стерео, глаза рядом в одной текстуре. Разрешение — на глаз
    // (по одной стороне): выше — чётче края блоков, но дороже для GPU.
    inline constexpr int   kWorldEyeRes      = 1280;

    // Меню висит на позе "прицела" (aim) левого контроллера; false — на позе рукояти (grip).
    // Смещение — в метрах, в осях контроллера: +X вправо, +Y вверх, -Z вперёд.
    inline constexpr bool  kAnchorToAimPose = true;
    inline constexpr float kMenuOffset[3]   = { 0.0f, 0.0f, 0.0f };

    // Клик — правым курком. Разные пороги нажатия и отпускания убирают дребезг.
    inline constexpr float kTriggerPress   = 0.55f;
    inline constexpr float kTriggerRelease = 0.35f;

    // Пока луч наведён на меню, игра не получает правый курок и правый стик
    // (клик по кнопке меню не стреляет/не хватает в самой игре).
    inline constexpr bool  kBlockGameInputOnMenu = true;

    // Прокрутка страниц правым стиком (вверх/вниз), пока луч на меню.
    inline constexpr bool  kStickScroll = true;

    // Minecraft: кнопка ПРАВОГО контроллера, которая переключает Place / Break, пока в руке блок.
    // None — переключать только в меню. Игра эту кнопку тоже видит.
    enum class EModeButton { None, A, B };
    inline constexpr EModeButton kModeButton = EModeButton::B;

    // Minecraft: сколько блоков можно поставить и как далеко "в воздухе" встаёт блок,
    // если луч ни во что не попал (м)
    inline constexpr int   kMaxBlocks     = 1000;
    inline constexpr float kAirPlaceM     = 1.5f;

    // Меняется на лету из Settings (сохраняется в файл настроек меню).
    inline float g_fPanelScale = 1.f;    // множитель физического размера панели
    inline bool  g_bMotion     = true;   // анимации меню (включая «пружину» появления)

    constexpr const char* ModeButtonName()
    {
        return kModeButton == EModeButton::A ? "A" : kModeButton == EModeButton::B ? "B" : nullptr;
    }

    constexpr const char* ToggleButtonName()
    {
        return kToggleButton == EToggleButton::X          ? "X"
             : kToggleButton == EToggleButton::Y          ? "Y"
             : kToggleButton == EToggleButton::StickClick ? "L-Stick"
             :                                              "Menu";
    }
}
