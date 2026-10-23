#pragma once

#include <stdint.h>

#include "imgui.h"

// Каталог блоков и атлас их текстур. Текстуры 16×16 лежат в коде (block_textures.inl),
// поэтому меню не зависит от ассетов игры: и иконки, и блоки в мире рисует ImGui.
namespace mc
{
    enum class ECategory : uint8_t { Natural, Wood, Building, Ores, Functional, Wool, NetherEnd, Count };
    const char* CategoryName(ECategory eCat);

    // Грани куба в осях Unity: +X восток, +Y верх, +Z север (вперёд)
    enum EFace : uint8_t { kPosX, kNegX, kPosY, kNegY, kPosZ, kNegZ, kFaceCount };

    enum EBlockFlags : uint8_t
    {
        kTranslucent = 1,   // есть прозрачные пиксели: видны задние грани, соседей не скрывает
        kPillar      = 2,   // бревно/колонна: ось по грани, на которую ставишь
        kFacing      = 4,   // лицевая сторона (тыква, печь) смотрит на игрока
        kGlow        = 8,   // светится: все грани без затенения
    };

    struct SBlockType
    {
        const char* pszName;
        ECategory   eCat;
        uint16_t    uTop, uSide, uBottom, uFront;   // тайлы атласа
        uint8_t     uFlags;
    };

    int               BlockCount();
    const SBlockType& Block(int iType);

    // Какой тайл на грани: с учётом оси колонны (0 X, 1 Y, 2 Z) и лицевой стороны.
    // bRotate — повернуть текстуру на 90° (волокна бревна вдоль его оси).
    uint16_t FaceTile(const SBlockType& type, int iFace, int iAxis, int iFacing, bool& bRotate);

    // Атлас: тайлы 16×16 с полем в 1 пиксель (повтор края), без мип-уровней. У каждого
    // ImGui-контекста свой (текстуру создаёт его бэкенд): создаётся при первом вызове
    // и хранится в io.UserData текущего контекста.
    ImTextureRef ContextAtlas();
    struct SUv { float u0, v0, u1, v1; };
    SUv TileUv(uint16_t uTile);
}
