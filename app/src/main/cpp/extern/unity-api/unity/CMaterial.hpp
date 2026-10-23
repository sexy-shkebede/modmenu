#pragma once

#include "CObject.hpp"
#include "CShader.hpp"
#include "CTexture.hpp"
#include "common/Color.hpp"
#include "../sdk_core.hpp"

#include "../sdk_fwd.hpp"

namespace usdk
{
    class CMaterial : public CObject
    {
    public:
        static il2cpp::CClass* GetIl2cppClass();
        static mono::CType*    GetMonoType();

        static CMaterial* Create(const char* pszName, CShader* pShader);
        static CMaterial* Create(const char* pszName, CMaterial* pMaterial);

        CShader*          GetShader();
        void              SetShader(CShader* pShader);

        CTexture*         GetMainTexture();
        void              SetMainTexture(CTexture* pTexture);

        Color             GetColor(const char* pszProperty = OBFUSCATE_STR("_Color"));
        void              SetColor(Color color, const char* pszProperty = OBFUSCATE_STR("_Color"));

        float             GetFloat(const char* pszProperty);
        void              SetFloat(const char* pszProperty, float fValue);

        CTexture*         GetTexture(const char* pszTexture);
        void              SetTexture(CTexture* pTexture, const char* pszTexture);
    };
} // usdk
