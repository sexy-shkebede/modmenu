#pragma once

namespace menu
{
    void Setup();                  // один раз после создания ImGui-контекста: шрифт и стиль
    void Draw(float fVisibility);  // каждый кадр, пока меню видно; 0..1 — фаза появления
    void DrawSprites();            // картинки луча в полосе под панелью (луч, прицел, подпись)
}
