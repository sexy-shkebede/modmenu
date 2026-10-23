#include "SString.hpp"
#include <string.h>
#include "../il2cpp/include.hpp"
#include "../sdk_core.hpp"

namespace mono
{
    SString* SString::Create(const char* utf8)
    {
        return (SString*)il2cpp::Api.string_new(utf8);
    }

    char* SString::ToUTF8()
    {
        if (m_iLength <= 0) return nullptr;

        const char16_t* p   = (const char16_t*)m_aChars;
        int             len = m_iLength;

        if (p[0] == 0xFEFF) { p++; len--; }

        size_t iCap = len * 3 + 1;
        if (iCap > 2048) iCap = 2048;

        static char s_aBufs[4][2048];
        static int  s_iBufIdx = 0;
        
        char* pszOut = s_aBufs[s_iBufIdx++ % 4];

        size_t j = 0;
        for (int i = 0; i < len && j + 4 < iCap; i++)
        {
            char16_t c = p[i];
            if (c < 0x0080)
            {
                pszOut[j++] = (char)(c & 0x00FF);
            }
            else if (c <= 0x07FF)
            {
                pszOut[j++] = (char)(((c >> 6) & 0x1F) | 0xC0);
                pszOut[j++] = (char)((c & 0x3F) | 0x80);
            }
            else if (c >= 0xD800 && c <= 0xDBFF && i + 1 < len)
            {
                uint32_t hi = c;
                uint32_t lo = p[++i];
                uint32_t cp = ((hi - 0xD800) << 10) | (lo - 0xDC00);
                cp += 0x10000;
                pszOut[j++] = (char)((cp >> 18) | 0xF0);
                pszOut[j++] = (char)(((cp >> 12) & 0x3F) | 0x80);
                pszOut[j++] = (char)(((cp >> 6)  & 0x3F) | 0x80);
                pszOut[j++] = (char)((cp & 0x3F) | 0x80);
            }
            else
            {
                pszOut[j++] = (char)(((c >> 12) & 0x0F) | 0xE0);
                pszOut[j++] = (char)(((c >> 6)  & 0x3F) | 0x80);
                pszOut[j++] = (char)((c & 0x3F) | 0x80);
            }
        }
        pszOut[j] = '\0';
        return pszOut;
    }
} // mono
