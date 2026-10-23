#include "CInput.hpp"
#include "../il2cpp/CClass.hpp"
#include "../mono/CType.hpp"
#include "../sdk_core.hpp"

namespace usdk
{
    il2cpp::CClass* CInput::GetIl2cppClass()
    {
        RESOLVE_SDK_IL2CPP_CLASS("Input", "UnityEngine", "UnityEngine.InputLegacyModule.dll");
    }

    mono::CType* CInput::GetMonoType()
    {
        RESOLVE_SDK_MONO_TYPE("UnityEngine.Input, UnityEngine.InputLegacyModule.dll");
    }

    STouch CInput::GetTouch(const uint8_t &index)
    {
        RESOLVE_SDK_METHOD(STouch (*)( uint8_t ), CInput::GetIl2cppClass(), "GetTouch", 1);
        return func(index);
    }
} // usdk
