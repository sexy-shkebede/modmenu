#pragma once

#include "imgui.h"

#include <stdint.h>

// Картинка мозга для меню: все нейроны FlyWire в проекции сзади (x вправо, y вниз; левое
// полушарие мухи — слева). Тусклый фон — плотность нейронов, яркое — кто спайкнул недавно.
// Живёт в контексте ImGui меню (рендер-поток).
namespace fly
{
    inline constexpr int kViewW = 384, kViewH = 192;

    // nullptr — мозг ещё не загружен. Первый вызов создаёт текстуру в текущем контексте ImGui.
    ImTextureData* BrainView();
    // Забрать спайки у симуляции, погасить старые, раз в ~1/15 с — новая картинка в текстуру
    void BrainViewUpdate(float fDt);
    // Где на картинке группа нейронов (0..1 по ширине и высоте); false — группы нет
    bool BrainViewGroup(const char* pszGroup, ImVec2& outUv);
}
