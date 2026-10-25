#pragma once

#include <stdint.h>

#include "unity/common/Vector3.hpp"
#include "unity/common/Quaternion.hpp"

// Тонкая прослойка к Unity через IL2CPP. Каждый нужный метод проверяется при поиске:
// если игра его вырезала, функция тихо не сработает. Вызывать из главного потока игры.
namespace ubridge
{
    // Точка и направление из осей головы (камеры Unity) — в мировые координаты игры.
    bool HeadToWorld(Vector3 posHead, Vector3 dirHead, Vector3& outPos, Vector3& outDir);

    // Риг игрока — родитель Camera.main (нет родителя — начало мира). Камера стоит в нём
    // на позе головы из трекинга, так что риг переводит пространство трекинга игры в мир.
    // camPos — где сейчас камера. fScale — масштаб игрока (единиц мира в метре трекинга).
    bool GetRig(Vector3& outPos, Quaternion& outRot, float& outScale, Vector3& outCamPos);

    // Physics.Raycast в мировых координатах игры (без триггеров и слоя Ignore Raycast)
    struct SHit { Vector3 point; Vector3 normal; float fDistance; };
    bool Raycast(Vector3 origin, Vector3 dir, float fMaxDistance, SHit& out);

    // Масштаб игрока: сколько единиц мира Unity в одном метре трекинга (обычно 1)
    float WorldScale();
}
