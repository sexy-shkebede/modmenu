#pragma once

// Функции мода, которые работают с объектами игры (Unity, IL2CPP).
namespace mods
{
    // Вызывается в главном потоке игры (UnityMain) на каждой синхронизации ввода OpenXR.
    // Трогать объекты Unity можно только отсюда: меню рисуется в рендер-потоке, где это запрещено.
    void Tick(float fDt);

    // Fly: держишь кнопку — летишь туда, куда указывает правая рука.
    struct SFly
    {
        bool  bEnabled = false;
        float fSpeed   = 10.f;   // м/с
    };
    inline SFly Fly;

    // Noclip: держишь ту же кнопку, что и для Fly, — столкновения игрока выключены.
    struct SNoclip    { bool bEnabled = false; };
    inline SNoclip Noclip;

    // Platforms: курок на руке — рука "хватается за воздух" и остаётся на месте в мире игры.
    struct SPlatforms { bool bEnabled = false; };
    inline SPlatforms Platforms;
}
