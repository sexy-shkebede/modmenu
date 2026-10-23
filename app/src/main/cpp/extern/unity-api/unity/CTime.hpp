#pragma once

#include "../sdk_fwd.hpp"

namespace usdk
{
    class CTime
    {
    public:
        static il2cpp::CClass* GetIl2cppClass();

	    static float GetTime();
	    static float GetTimeSinceLevelLoad();
	    static float GetDeltaTime();
	    static float GetUnscaledTime();
	    static float GetFixedUnscaledTime();
	    static float GetUnscaledDeltaTime();
	    static float GetFixedDeltaTime();
	    static float GetSmoothDeltaTime();
	    static float GetTimeScale();
	    static void  SetTimeScale(float fValue);
	    static int   GetFrameCount();
	    static float GetRealtimeSinceStartup();
    };
} // usdk
