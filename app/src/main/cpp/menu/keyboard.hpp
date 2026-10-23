#pragma once

// Экранная клавиатура в стиле M3: модальный bottom sheet поверх меню (scrim, текстовое поле,
// раскладка). Нажатие по затемнению — отмена.
namespace keyboard
{
    using FnDone = void (*)(const char* pszText);

    void Open(const char* pszTitle, const char* pszInitial, int nMaxLen, FnDone pfnDone);
    bool IsOpen();
    void Draw(float W, float H);   // каждый кадр, после окна меню
}
