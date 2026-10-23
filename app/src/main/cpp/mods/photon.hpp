#pragma once

// Photon PUN 2 через IL2CPP — только чтобы узнать, в какой комнате игрок.
// Вызывать из главного потока игры.
namespace photon
{
    enum class ERoom
    {
        NoPun,       // игра не на PUN 2 (или он вырезан) — ограничений нет
        NotInRoom,
        Private,     // комната скрыта из поиска (IsVisible == false): "по коду"
        Public,
    };
    ERoom GetRoom();
}
