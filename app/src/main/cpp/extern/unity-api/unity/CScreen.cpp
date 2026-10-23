#include "CScreen.hpp"
#include "../il2cpp/CClass.hpp"
#include "../il2cpp/CObject.hpp"
#include "../mono/CType.hpp"
#include "../mono/SString.hpp"
#include "../mono/SArray.hpp"
#include "../sdk_core.hpp"

namespace usdk
{
    il2cpp::CClass* CScreen::GetIl2cppClass()
    {
        RESOLVE_SDK_IL2CPP_CLASS("Screen", "UnityEngine", "UnityEngine.CoreModule.dll");
    }

    mono::CType* CScreen::GetMonoType()
    {
        RESOLVE_SDK_MONO_TYPE("UnityEngine.Screen, UnityEngine.CoreModule.dll");
    }

    SResolution CScreen::GetResolution() 
    {
        static int (*func0)() = (decltype(func0))(GetIl2cppClass()->GetMethodFromName(OBFUSCATE_STR("get_width"),  0)->GetMethodPointer());
        static int (*func1)() = (decltype(func1))(GetIl2cppClass()->GetMethodFromName(OBFUSCATE_STR("get_height"), 0)->GetMethodPointer());
        return SResolution(func0(), func1());
    }

    void CScreen::SetResolution(int32_t w, int32_t h) 
    {
        RESOLVE_SDK_METHOD(void (*)( int32_t, int32_t, bool ), GetIl2cppClass(), "SetResolution", 3);
        func(w, h, true);
    }

    void CScreen::SetResolution(SResolution res) 
    {
        CScreen::SetResolution(res.m_iWidth, res.m_iHeight);
    }
} // usdk
