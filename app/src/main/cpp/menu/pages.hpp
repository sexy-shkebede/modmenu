#pragma once

#include "ui.hpp"

// Страницы меню. Добавить свою: функция + строка в kPages.
// iGroup 0 — основной список сверху, 1 — служебные пункты, прижатые к низу.
namespace pages
{
    struct SPage
    {
        const char* pszName;
        ui::EIcon   eIcon;
        void      (*pfnDraw)();
        int         iGroup;
    };

    void Movement();
    void Misc();
    void Overpowered();
    void Photon();
    void Settings();
    void Info();

    inline constexpr SPage kPages[] =
    {
        { "Movement",    ui::EIcon::Flight,   Movement,    0 },
        { "Misc",        ui::EIcon::GridView, Misc,        0 },
        { "Overpowered", ui::EIcon::Bolt,     Overpowered, 0 },
        { "Photon",      ui::EIcon::Cloud,    Photon,      0 },
        { "Settings",    ui::EIcon::Settings, Settings,    1 },
        { "Info",        ui::EIcon::Info,     Info,        1 },
    };
    inline constexpr int kCount = (int)(sizeof(kPages) / sizeof(kPages[0]));
}
