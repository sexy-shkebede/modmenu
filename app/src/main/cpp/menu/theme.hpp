#pragma once

// Динамический цвет Material 3: из одного исходного цвета (seed) строятся тональные палитры
// в пространстве HCT, а из них — все цветовые роли схемы, как в Material Theme Builder и Android 12+.
namespace theme
{
    enum class EStyle { TonalSpot, Vibrant, Neutral, Monochrome };
    inline constexpr const char* kStyleNames[] = { "Tonal", "Vibrant", "Neutral", "Mono" };
    inline constexpr int         kStyleCount   = 4;

    // Исходные цвета — палитра Material Design (оттенки 500) и базовый цвет M3.
    struct SSeed { unsigned rgb; const char* pszName; };
    inline constexpr SSeed kSeeds[] =
    {
        { 0x6750A4, "Baseline"    }, { 0xF44336, "Red"         }, { 0xE91E63, "Pink"        }, { 0x9C27B0, "Purple" },
        { 0x673AB7, "Deep Purple" }, { 0x3F51B5, "Indigo"      }, { 0x2196F3, "Blue"        }, { 0x03A9F4, "Light Blue" },
        { 0x00BCD4, "Cyan"        }, { 0x009688, "Teal"        }, { 0x4CAF50, "Green"       }, { 0x8BC34A, "Light Green" },
        { 0xCDDC39, "Lime"        }, { 0xFFEB3B, "Yellow"      }, { 0xFFC107, "Amber"       }, { 0xFF9800, "Orange" },
        { 0xFF5722, "Deep Orange" }, { 0x795548, "Brown"       }, { 0x607D8B, "Blue Grey"   },
    };
    inline constexpr int kSeedCount = (int)(sizeof(kSeeds) / sizeof(kSeeds[0]));

    // Строит схему и переводит на неё роли ui::role (с анимацией — за 300 мс).
    void Apply(unsigned seed, EStyle style, bool bDark, bool bAnimate);
    void Tick(float dt);   // каждый кадр: анимация смены темы
    // Цвета кружка-превью (primary, secondary, tertiary) — как в выборе темы на Android.
    void Preview(unsigned seed, EStyle style, bool bDark, unsigned out[3]);
    // Роль по порядку UI_COLOR_ROLES — для проверки схемы в тесте
    void Build(unsigned seed, EStyle style, bool bDark, unsigned* pOutRoles);
}
