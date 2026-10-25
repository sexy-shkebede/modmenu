#pragma once

#include <stdint.h>
#include <string>
#include <vector>

// Коннектом мухи из fly_brain.bin (собирает tools/fly/make_brain.py из данных FlyWire v783 и
// таблицы связей Shiu et al. 2024): исходящие связи каждого нейрона, именованные группы
// входов/выходов (как в fly-connectome/tiktok/senses.py) и координаты нейронов.
namespace fly
{
    struct SConnectome
    {
        SConnectome() = default;
        SConnectome(const SConnectome&) = delete;              // указатели смотрят внутрь своих данных
        SConnectome& operator=(const SConnectome&) = delete;

        uint32_t        N = 0, M = 0;          // нейронов, связей
        const uint32_t* pCptr  = nullptr;      // исходящие связи нейрона j: pSyn[pCptr[j] .. pCptr[j + 1])
        const uint32_t* pSyn   = nullptr;      // биты 0..17 — постсинаптический нейрон, 18..31 — знак × число синапсов
        const int16_t*  pPos   = nullptr;      // N × 3: x, y, z в единицах 0.25 мкм (FlyWire: x вправо, y вниз)
        const uint8_t*  pSuper = nullptr;      // класс нейрона (1 optic, 2 central, 3 sensory, ...)

        struct SGroup { std::string sName; std::vector<uint32_t> idx; };
        std::vector<SGroup> groups;
        const std::vector<uint32_t>* Group(const char* pszName) const;

        static uint32_t Post(uint32_t s)   { return s & 0x3FFFFu; }
        static int32_t  Weight(uint32_t s) { return (int32_t)s >> 18; }

        // pProgress (0..1) обновляется по ходу чтения — для меню; вызывать из фонового потока
        bool Load(const char* pszPath, std::string& sError, volatile float* pProgress = nullptr);

    private:
        std::vector<uint32_t> m_Data;          // распакованное содержимое (выровнено по 4 байта)
    };
}
