#pragma once

#include "body.hpp"
#include "imgui.h"

struct SWorldEye;

// Муха и капли сахара в слое мира: треугольники ImGui, без шейдеров и ассетов игры.
namespace fly
{
    // Риг игрока: родитель камеры Unity (мир → пространство трекинга игры)
    struct SRig
    {
        bool  bValid = false;
        float aPos[3] = { 0.f, 0.f, 0.f };
        float aRot[4] = { 0.f, 0.f, 0.f, 1.f };
        float fScale  = 1.f;
    };

    struct SScene
    {
        SRig  rig;
        bool  bFly = false;
        float aOrigin[3] = {};         // мир Unity: точка на полу под грудью мухи
        float fYaw = 0.f, fPitch = 0.f, fRoll = 0.f;   // рад; yaw + — к +X мира
        float fMmToWorld = 0.03f;      // единиц мира в миллиметре мухи
        SPose pose;
        int   nDrops = 0;
        float aDrops[8][4] = {};       // капли сахара: x, y, z (мир), радиус (мир)
        char  szLabel[96] = {};        // подпись над мухой ("" — нет)
        char  szLabel2[96] = {};
    };

    void SetupWorldContext();          // шрифт подписи — для ImGui-контекста слоя мира
    void DrawScene(ImDrawList* dl, const SScene& scene, const SWorldEye* pEyes, int nEyes);
}
