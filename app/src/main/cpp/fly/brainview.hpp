#pragma once

#include "imgui.h"

#include <stdint.h>

// Картинки нервной системы для меню.
//  • Мозг: все нейроны FlyWire в проекции сзади (x вправо, y вниз; левое полушарие мухи — слева).
//  • Цепочка (BANC): все нейроны брюшной нервной цепочки сверху, голова слева, левый бок мухи — снизу.
// Тусклый фон — плотность нейронов, яркое — кто сейчас активен (спайки мозга, частоты цепочки).
// Живёт в контексте ImGui меню (рендер-поток).
namespace fly
{
    inline constexpr int kViewW = 384, kViewH = 192;
    inline constexpr int kCordW = 256, kCordH = 150;

    // nullptr — мозг ещё не загружен. Первый вызов создаёт текстуру в текущем контексте ImGui.
    ImTextureData* BrainView();
    // Забрать спайки у симуляции, погасить старые, раз в ~1/15 с — новая картинка в текстуру
    void BrainViewUpdate(float fDt);
    // Где на картинке группа нейронов (0..1 по ширине и высоте); false — группы нет
    bool BrainViewGroup(const char* pszGroup, ImVec2& outUv);

    // То же для цепочки; nullptr — цепочки нет (или ещё не готова)
    ImTextureData* CordView();
    void CordViewUpdate(float fDt);
    bool CordViewGroup(const char* pszGroup, ImVec2& outUv);
}
