#include "CEvent.hpp"
#include "../il2cpp/CClass.hpp"
#include "../il2cpp/CObject.hpp"
#include "../mono/CType.hpp"
#include "../mono/SString.hpp"
#include "../mono/SArray.hpp"
#include "../sdk_core.hpp"

namespace usdk
{
    il2cpp::CClass* CEvent::GetIl2cppClass()
    {
        RESOLVE_SDK_IL2CPP_CLASS("Event", "UnityEngine", "UnityEngine.IMGUIModule.dll");
    }

    /*
    mono::CType* CEvent::GetMonoType()
    {
        RESOLVE_SDK_MONO_TYPE("UnityEngine.Event, UnityEngine.IMGUIModule.dll");
    }
    */

    EEventType CEvent::GetType() 
    {
        RESOLVE_SDK_METHOD(EEventType (*)( CEvent* ), this->GetIl2cppClass(), "get_rawType", 0);
        return func(this);
    }

    CEvent* CEvent::GetCurrent() 
    {
        RESOLVE_SDK_METHOD(CEvent* (*)(  ), GetIl2cppClass(), "get_current", 0);
        return func();
    }
} // usdk
