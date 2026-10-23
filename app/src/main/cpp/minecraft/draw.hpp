#pragma once

#include "imgui.h"
#include "world.hpp"

struct SWorldEye;

// Блоки рисует ImGui: текстурированные треугольники в draw list, без шейдеров игры.
namespace mc
{
    // Пиксельные текстуры — без размытия: переключить сэмплер бэкенда (и обязательно вернуть линейный)
    void SetSamplerNearest(ImDrawList* dl, bool bNearest);

    // Иконка блока, как в инвентаре Minecraft: изометрический куб, fSize — высота шестиугольника
    void DrawIcon(ImDrawList* dl, ImTextureRef atlas, int iType, ImVec2 center, float fSize, float fAlpha = 1.f);

    // Блоки в мире: каждый глаз — в свой участок текстуры слоя, с перспективой и порядком
    // "от дальних к ближним". Сэмплер переключает сам.
    void DrawWorld(ImDrawList* dl, ImTextureRef atlas, const SWorld& world, const SWorldEye* pEyes, int nEyes);
}
