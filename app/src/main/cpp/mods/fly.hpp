#pragma once

#include "../fly/sim.hpp"

#include <string>

struct SWorldEye;

// Муха в игре: мозг (fly/sim) решает, нервная цепочка (если есть fly_vnc.bin) двигает ноги,
// тело (fly/body) показывает, слой мира рисует.
// Пороги и органы чувств — как в fly-connectome (tiktok/senses.py, show.py), а мир — игра:
// муха видит её геометрию лучами-фасетками, ходит по любым поверхностям (пол, стены, потолок),
// летает, садится, чистится от пыли, ищет еду, когда голодна. Капли сахара ставит игрок,
// угроза — его руки и голова, приманка — точка луча.
// Меню и хук (рендер-поток) читают снимок и ставят команды; Unity трогает только Tick
// в главном потоке игры.
namespace flymod
{
    enum class ETool { Off, Spawn, Sugar, Lure };                     // что делает луч правой руки
    enum class EStim { Puff, Sugar, Bitter, LoomL, LoomR, Sound };    // стимулы прямо в нейроны

    enum class EState { None, Walk, Rest, Seek, Threat, Groom, Eat, Proboscis, Takeoff, Flight, Dodge, Landing, Startle, Swatted, Stuck };

    struct SOptions
    {
        float fSize   = 10.f;     // во сколько раз муха больше настоящей (2.5 мм); 1 — настоящая
        bool  bLabels = true;     // подпись над мухой
    };

    struct SStatus
    {
        bool        bFly      = false;
        bool        bFlying   = false;
        EState      eState    = EState::None;
        const char* pszState  = "";      // что делает (для меню и подписи)
        const char* pszCause  = "";      // какие нейроны / правило это решают
        float       aOut[fly::kOutputCount] = {};   // сглаженные частоты выходов, Гц на нейрон
        float       aIn[fly::kInputCount]   = {};   // что подаём на входы, Гц
        float       fTurn     = 0.f;     // −1..1, + — влево
        float       fHunger   = 0.f, fDust = 0.f;
        int         nDrops    = 0, nEaten = 0, nFlights = 0;
        // нервная цепочка (BANC): мотонейроны ног, крыльев, прыжка
        bool        bVnc      = false;
        fly::SVncOut vnc;
        float       fStepHz   = 0.f;                // ритм шагов
        int         nRhythm   = 0;                  // у скольких ног ритм сейчас
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
