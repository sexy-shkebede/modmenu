#pragma once

#include <string>

struct SWorldEye;

// Minecraft в игре: блоки ставятся лучом правой руки и рисуются слоем мира (ImGui).
// Меню и хук (рендер-поток) читают снимок и ставят команды; Unity трогает только Tick
// в главном потоке игры.
namespace minecraft
{
    enum class EMode { Place, Break };

    struct SOptions
    {
        float fSize        = 0.5f;    // ребро нового блока, м (в масштабе игрока)
        bool  bSnap        = true;    // по сетке размера блока
        bool  bSolid       = true;    // коллайдеры: на блоках можно стоять (только не в публичной комнате)
        bool  bHideBehind  = true;    // блоки за стенами игры прячутся
        float fBrightness  = 100.f;   // %, под тёмные игры
    };

    struct SStatus
    {
        int         iSelected       = -1;   // блок в руке (индекс каталога), -1 — рука пустая
        EMode       eMode           = EMode::Place;
        int         nBlocks         = 0;
        bool        bCanUndo        = false;
        bool        bSolidSupported = true;  // игра не вырезала нужные методы Unity
        bool        bPublicRoom     = false; // публичная комната Photon: коллайдеры выключены
        std::string sMessage;                // результат последнего действия (для snackbar)
        int         iMessageId      = 0;
    };

    // ---- меню ----
    SStatus  GetStatus();
    SOptions GetOptions();
    void     SetOptions(const SOptions& opt);
    void     Select(int iType);              // -1 — убрать блок из руки, луч гаснет
    void     SetMode(EMode eMode);
    void     Undo();
    void     ClearAll();
    std::string BeamLabel();                 // подпись у прицела

    // ---- хук, рендер-поток ----
    bool WorldActive();
    void DrawWorld(const SWorldEye* aEyes, int nEyes);   // внутри кадра ImGui слоя мира

    // ---- главный поток игры ----
    void Tick(float fDt);
}
