#pragma once

#include <stdint.h>
#include <vector>

// Снимок мира блоков для рендер-потока. Позиции — в мировых координатах Unity.
namespace mc
{
    struct SPlaced
    {
        uint32_t uId;
        float    c[3];            // центр
        float    fHalf;           // половина ребра
        uint16_t uType;           // индекс в каталоге (blocks.hpp)
        uint8_t  uAxis;           // ось колонны: 0 X, 1 Y, 2 Z
        uint8_t  uFacing;         // грань с лицевой текстурой (EFace)
        uint8_t  uHiddenFaces;    // биты EFace: грань закрыта непрозрачным соседом
        float    fAlpha;          // 0..1: прячется за стенами игры
        float    fScale;          // анимация появления 0..1
    };

    // Риг игрока: родитель камеры Unity. Камера стоит в нём на позе головы из OpenXR,
    // поэтому мир → пространство трекинга игры = обратное преобразование рига.
    struct SRig
    {
        bool  bValid = false;
        float aPos[3] = { 0.f, 0.f, 0.f };
        float aRot[4] = { 0.f, 0.f, 0.f, 1.f };
        float fScale  = 1.f;
    };

    // Куда смотрит луч: призрак будущего блока или подсвеченный блок под ломание
    struct SCursor
    {
        bool     bShow  = false;
        bool     bGhost = false;  // false — только рамка (режим Break)
        float    c[3]   = { 0.f, 0.f, 0.f };
        float    fHalf  = 0.f;
        uint16_t uType  = 0;
        uint8_t  uAxis  = 1, uFacing = 0;
    };

    struct SWorld
    {
        std::vector<SPlaced> blocks;
        SRig                 rig;
        SCursor              cursor;
        float                fBrightness = 1.f;
    };
}
