#pragma once

#include <stdint.h>
#include <string>
#include <vector>

#include "unity/common/Vector3.hpp"

// Тонкая прослойка к Unity через IL2CPP. Каждый нужный метод проверяется при поиске:
// если игра его вырезала, функция тихо не сработает. Вызывать из главного потока игры.
namespace ubridge
{
    // Сдвигает локального игрока на dirCamera * fDistance (dirCamera — в осях камеры Unity).
    // Если у игрока есть Rigidbody (физическое передвижение) — двигаем его и гасим скорость,
    // иначе двигаем корень иерархии камеры. false — IL2CPP или камера недоступны.
    bool MoveLocalPlayer(Vector3 dirCamera, float fDistance);

    // Сдвиг игрока на вектор в мировых координатах игры (тот же Rigidbody или корень),
    // скорость Rigidbody гасится — игрок "висит", даже если сдвиг нулевой.
    bool MovePlayerBy(Vector3 delta);

    // Столкновения игрока: Rigidbody.detectCollisions и CharacterController, если они есть.
    bool SetPlayerCollisions(bool bEnabled);

    // Точка и направление из осей головы (камеры Unity) — в мировые координаты игры.
    bool HeadToWorld(Vector3 posHead, Vector3 dirHead, Vector3& outPos, Vector3& outDir);

    // Physics.Raycast в мировых координатах игры (без триггеров и слоя Ignore Raycast)
    struct SHit { Vector3 point; Vector3 normal; float fDistance; };
    bool Raycast(Vector3 origin, Vector3 dir, float fMaxDistance, SHit& out);

    // Масштаб игрока: сколько единиц мира Unity в одном метре трекинга (обычно 1)
    float WorldScale();

    // Префабы игры, загруженные в память: корни вне сцен, не скрытые, без камеры внутри
    // (это риги — перехватят картинку) и без компонента pSkipType (например, PhotonView —
    // такие спавнит Photon). hObject — GC-handle, освобождать il2::FreeGCHandle.
    struct SPrefab { std::string sName; uint32_t hObject = 0; };
    bool FindLoadedPrefabs(std::vector<SPrefab>& out, void* pSkipType, size_t nMax);

    // Локальная копия префаба (Object.Instantiate): видна только тебе
    bool InstantiateLocal(uint32_t hPrefab, Vector3 at);
}
