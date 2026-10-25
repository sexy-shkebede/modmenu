#include "mods.hpp"
#include "fly.hpp"
#include <android/log.h>

namespace mods
{
    // Исключение из управляемого кода (например, объект игры уже уничтожен) не должно
    // уронить игру: ловим, пишем в logcat не чаще раза в секунду и пробуем на следующем кадре.
    template <typename F> static void Guarded(const char* pszName, float fDt, float& fSinceLog, F&& fn)
    {
        fSinceLog += fDt;
        try { fn(); }
        catch (...)
        {
            if (fSinceLog >= 1.f)
            {
                __android_log_print(ANDROID_LOG_WARN, "shkbd", "mods: exception from game code in %s swallowed", pszName);
                fSinceLog = 0.f;
            }
        }
    }

    void Tick(float fDt)
    {
        static float s_fFly = 1.f;
        Guarded("fly", fDt, s_fFly, [&] { flymod::Tick(fDt); });
    }
}
