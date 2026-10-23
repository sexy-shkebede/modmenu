#pragma once

// Настройки меню. Хранятся в файле в каталоге данных игры (/data/data/<пакет>/files),
// поэтому переживают перезапуск.
namespace settings
{
    struct SSettings
    {
        int   iSeed    = 0;       // theme::kSeeds
        int   iStyle   = 0;       // theme::EStyle
        bool  bDark    = true;
        float fSizePct = 100.f;   // размер панели, %
        bool  bMotion  = true;    // анимации интерфейса
    };

    SSettings& Get();
    void  Load();                 // один раз при запуске: читает файл и применяет (и настройки блоков Minecraft)
    void  Changed(bool bTheme);   // после изменения: применить (тему — с анимацией) и сохранить
    void  Tick(float dt);         // каждый кадр меню: отложенное сохранение, плавный размер
    float PanelScale();           // множитель размера панели для хука
}
