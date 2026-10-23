#include "CTime.hpp"
#include "../il2cpp/CClass.hpp"
#include "../il2cpp/CObject.hpp"
#include "../mono/CType.hpp"
#include "../mono/SString.hpp"
#include "../mono/SArray.hpp"
#include "../sdk_core.hpp"

namespace usdk
{
    il2cpp::CClass* CTime::GetIl2cppClass()
    {
        RESOLVE_SDK_IL2CPP_CLASS("Time", "UnityEngine", "UnityEngine.CoreModule.dll");
    }

	float CTime::GetTime()
    {
        RESOLVE_SDK_METHOD(float (*)(  ), GetIl2cppClass(), "get_time", 0);
        return func();
    }

	float CTime::GetTimeSinceLevelLoad()
    {
        RESOLVE_SDK_METHOD(float (*)(  ), GetIl2cppClass(), "get_timeSinceLevelLoad", 0);
        return func();
    }

	float CTime::GetDeltaTime()
    {
        RESOLVE_SDK_METHOD(float (*)(  ), GetIl2cppClass(), "get_deltaTime", 0);
        return func();
    }

	float CTime::GetUnscaledTime()
    {
        RESOLVE_SDK_METHOD(float (*)(  ), GetIl2cppClass(), "get_unscaledTime", 0);
        return func();
    }

	float CTime::GetFixedUnscaledTime()
    {
        RESOLVE_SDK_METHOD(float (*)(  ), GetIl2cppClass(), "get_fixedUnscaledTime", 0);
        return func();
    }

	float CTime::GetUnscaledDeltaTime()
    {
        RESOLVE_SDK_METHOD(float (*)(  ), GetIl2cppClass(), "get_unscaledDeltaTime", 0);
        return func();
    }

	float CTime::GetFixedDeltaTime()
    {
        RESOLVE_SDK_METHOD(float (*)(  ), GetIl2cppClass(), "get_fixedDeltaTime", 0);
        return func();
    }

	float CTime::GetSmoothDeltaTime()
    {
        RESOLVE_SDK_METHOD(float (*)(  ), GetIl2cppClass(), "get_smoothDeltaTime", 0);
        return func();
    }

	float CTime::GetTimeScale()
    {
        RESOLVE_SDK_METHOD(float (*)(  ), GetIl2cppClass(), "get_timeScale", 0);
        return func();
    }

	void CTime::SetTimeScale(float fValue)
    {
        RESOLVE_SDK_METHOD(void (*)( float ), GetIl2cppClass(), "set_timeScale", 0);
        func(fValue);
    }

	int CTime::GetFrameCount()
    {
        RESOLVE_SDK_METHOD(int (*)(  ), GetIl2cppClass(), "get_frameCount", 0);
        return func();
    }

	float CTime::GetRealtimeSinceStartup()
    {
        RESOLVE_SDK_METHOD(float (*)(  ), GetIl2cppClass(), "get_realtimeSinceStartup", 0);
        return func();
    }
}
