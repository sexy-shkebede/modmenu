#include "CTextMeshPro.hpp"
#include "../il2cpp/CClass.hpp"
#include "../il2cpp/CObject.hpp"
#include "../mono/CType.hpp"
#include "../mono/SString.hpp"
#include "../mono/SArray.hpp"
#include "../sdk_core.hpp"

namespace usdk
{
    il2cpp::CClass* CTMP_Text::GetIl2cppClass()
    {
        RESOLVE_SDK_IL2CPP_CLASS("TMP_Text", "TMPro", "Unity.TextMeshPro.dll");
    }

    mono::CType* CTMP_Text::GetMonoType()
    {
        RESOLVE_SDK_MONO_TYPE("TMPro.TextMeshPro, Unity.TextMeshPro.dll");
    }

    void CTMP_Text::SetText(const char* pszText)
    {
        RESOLVE_SDK_METHOD(void (*)( CTMP_Text*, mono::SString*, bool ), this->GetIl2cppClass(), "SetText", 2);
        func(this, mono::SString::Create(pszText), false);
    }

    void CTMP_Text::SetFont(void* pFontAsset)
    {
        RESOLVE_SDK_METHOD(void (*)( CTMP_Text*, void* ), this->GetIl2cppClass(), "set_font", 1);
        func(this, pFontAsset);
    }

    void CTMP_Text::SetFontSize(float fSize)
    {
        RESOLVE_SDK_METHOD(void (*)( CTMP_Text*, float ), this->GetIl2cppClass(), "set_fontSize", 1);
        func(this, fSize);
    }

    void CTMP_Text::SetColor(Color color)
    {
        RESOLVE_SDK_METHOD(void (*)( CTMP_Text*, Color ), this->GetIl2cppClass(), "set_color", 1);
        func(this, color);
    }

    void CTMP_Text::SetAligment(ETextAlignmentOptions iAlignmentOptions)
    {
        RESOLVE_SDK_METHOD(void (*)( CTMP_Text*, int ), this->GetIl2cppClass(), "set_alignment", 1);
        func(this, iAlignmentOptions);
    }

    void CTMP_Text::SetWrapping(const bool bState)
    {
        RESOLVE_SDK_METHOD(void (*)( CTMP_Text*, bool ), this->GetIl2cppClass(), "set_enableWordWrapping", 1);
        func(this, bState);
    }
} // usdk
