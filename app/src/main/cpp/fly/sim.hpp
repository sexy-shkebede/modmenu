#pragma once

#include "connectome.hpp"
#include "vnc.hpp"

#include <stdint.h>
#include <string>
#include <vector>

// Нервная система мухи в своих потоках: мозг (fly_brain.bin, FAFB) считает в реальном времени
// (× масштаб времени), принимает частоты органов чувств и отдаёт средние частоты выходных нейронов.
// Если есть fly_vnc.bin (BANC), рядом идёт брюшная нервная цепочка: нисходящие нейроны мозга
// ведут её, мотонейроны ног, крыльев и прыжка — её выход, восходящие нейроны — обратно в мозг.
// Все функции — потокобезопасные.
namespace fly
{
    // Входы и выходы — группы нейронов из fly_brain.bin (как SENSES / READOUTS в senses.py; запах —
    // добавлен). Группы нет в старом файле — вход просто молчит. kWalkDrive — BDN2 (DNg100),
    // нисходящий "иди вперёд": его ведёт поведение (правило тела), шаги делает цепочка
    enum EInput  { kObjL, kObjR, kLoomL, kLoomR, kPuff, kWind, kSugar, kBitter, kHearA, kHearB, kOdorL, kOdorR, kWalkDrive, kInputCount };
    enum EOutput { kSteerL, kSteerR, kHeadL, kHeadR, kGF, kFear, kMN9, kGroom, kWalk, kBack, kHear2, kOutputCount };
    const char* InputGroup(int i);
    const char* OutputGroup(int i);

    enum class ELoad { Idle, Loading, Ready, Failed };

    struct SSimInfo
    {
        ELoad       eLoad = ELoad::Idle;
        float       fProgress = 0.f;
        std::string sPath, sError;   // откуда загружен / почему не вышло
        uint32_t    nNeurons = 0, nConnections = 0;
        double      dSimMs   = 0.0;  // сколько прожил мозг
        float       fSpeed   = 0.f;  // во сколько раз быстрее реального времени он мог бы считать
        uint32_t    nAwake   = 0;    // нейронов в счёте шаг за шагом
        uint32_t    nActive  = 0;    // спайкнувших за последние 100 мс
        // брюшная нервная цепочка (fly_vnc.bin)
        ELoad       eVnc = ELoad::Idle;
        std::string sVncPath, sVncError;
        uint32_t    nVncNeurons = 0, nVncConnections = 0, nVncDn = 0, nVncAn = 0;
        float       fVncSpeed = 0.f; // во сколько раз быстрее реального времени
    };

    // В фоне: первый найденный fly_brain.bin, потом fly_vnc.bin — вшитый или из тех же папок
    void     SimLoad(const std::vector<std::string>& paths);
    SSimInfo SimInfo();
    void     SimRun(bool bRun);                    // false — мозг стоит (мухи нет)
    void     SimSetTimeScale(float fScale);        // 1 — реальное время, 0.25 — замедление ×4
    void     SimSetInputs(const float aHz[kInputCount]);
    void     SimReset();                           // мембраны, токи, адаптация — в покой
    // Средние частоты групп выходов (Гц на нейрон) за время с прошлого вызова; dMs — сколько
    // прожил мозг за это время. false — мозг не шёл.
    bool     SimReadout(float aHz[kOutputCount], double& dMs);
    // Спайки с прошлого вызова (номера нейронов) — для картинки мозга
    void     SimTakeSpikes(std::vector<uint32_t>& out);
    // Коннектом после загрузки (не меняется) или nullptr
    const SConnectome* SimConnectome();

    // Цепочка: что делают ноги, крылья, прыжок. false — цепочки нет (или ещё не готова)
    bool     SimVnc(SVncOut& out);
    // Частоты нейронов цепочки (Гц) — для картинки; данные цепочки (не меняются) или nullptr
    void     SimVncRates(std::vector<float>& out);
    const SVncData* SimVncData();
}
