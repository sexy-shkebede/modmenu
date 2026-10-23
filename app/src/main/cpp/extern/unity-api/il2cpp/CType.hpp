#pragma once

#include <cstdint>

namespace il2cpp
{
    class CClass;

    class CType
    {
    public:
        uint16_t m_uAttrs;

        CClass* GetClassOrElementClass() const;
    };
} // il2cpp
