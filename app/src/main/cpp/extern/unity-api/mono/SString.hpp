#pragma once

#include <stdint.h>
#include "../il2cpp/CClass.hpp"

namespace mono
{
    struct SString
    {
        char pad[sizeof(void*) * 2];
    public:
        int32_t m_iLength;
        char    m_aChars[0];

        static SString* Create(const char* utf8);
        char* ToUTF8();
    };
} // mono
