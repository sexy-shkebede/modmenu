#pragma once

#include "../fly/sim.hpp"

#include <string>

struct SWorldEye;

// Муха в игре: мозг (fly/sim) решает, тело (fly/body) показывает, слой мира рисует.
// Органы чувств и правила тела — как в fly-connectome/show.py, только мир — игра:
// капли сахара ставит игрок, угроза — его руки, приманка — точка луча.
// Меню и хук (рендер-поток) читают снимок и ставят команды; Unity трогает только Tick
// в главном потоке игры.
namespace flymod
{
    enum class ETool { Off, Spawn, Sugar, Lure };                     // что делает луч правой руки
    enum class EStim { Puff, Sugar, Bitter, LoomL, LoomR, Sound };    // стимулы прямо в нейроны

    enum class EState { None, Walk, Rest, Avoid, Seek, Threat, Groom, Eat, Proboscis, Flight, Startle, Swatted };

    struct SOptions
    {
        float fSize      = 30.f;    // во сколько раз муха больше настоящей (2.5 мм)
        float fTimeScale = 100.f;   // %, 100 — реальное время
        bool  bWander    = true;    // бродит сама; выключено — идёт только к еде, приманке, от угрозы
        bool  bLabels    = true;    // подпись над мухой
        bool  bSwat      = true;    // быстрая рука, догнавшая муху до взлёта, её прихлопывает
    };

    struct SStatus
    {
        bool        bFly      = false;
        EState      eState    = EState::None;
        const char* pszState  = "";      // что делает (для меню и подписи)
        const char* pszCause  = "";      // какие нейроны / правило это решают
        float       aOut[fly::kOutputCount] = {};   // сглаженные частоты выходов, Гц на нейрон
        float       aIn[fly::kInputCount]   = {};   // что подаём на входы, Гц
        float       fTurn     = 0.f;     // −1..1, + — влево
        float       fHunger   = 0.f;
        int         nDrops    = 0, nEaten = 0, nTakeoffs = 0;
        ETool       eTool     = ETool::Off;
        std::string sMessage;
        int         iMessageId = 0;
    };

    // ---- меню ----
    void        StartBrain();             // начать загрузку fly_brain.bin (повторные вызовы — ничего)
    SStatus     GetStatus();
    SOptions    GetOptions();
    void        SetOptions(const SOptions& opt);
    void        SetTool(ETool eTool);
    void        Stimulate(EStim eStim);
    void        RemoveFly();
    void        ClearSugar();
    void        ResetBrain();
    std::string BeamLabel();              // подпись у прицела
    std::string BrainHint();              // куда положить fly_brain.bin

    // ---- хук, рендер-поток ----
    void SetupWorld();                    // шрифт подписи в контексте слоя мира
    bool WorldActive();
    void DrawWorld(const SWorldEye* aEyes, int nEyes);

    // ---- главный поток игры ----
    void Tick(float fDt);
}
