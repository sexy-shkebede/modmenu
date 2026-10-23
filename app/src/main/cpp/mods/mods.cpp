#include "mods.hpp"
#include "photon.hpp"
#include <android/log.h>

namespace mods
{
    // movement.cpp
    void TickFly(float fDt);
    void TickNoclip();
    void TickPlatforms();

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
        static float s_fFly = 1.f, s_fNoclip = 1.f, s_fPlatforms = 1.f, s_fPhoton = 1.f;
        Guarded("noclip",    fDt, s_fNoclip,    [&] { TickNoclip(); });
        Guarded("platforms", fDt, s_fPlatforms, [&] { TickPlatforms(); });
        Guarded("fly",       fDt, s_fFly,       [&] { TickFly(fDt); });
        Guarded("photon", fDt, s_fPhoton, [&] { photon::Tick(fDt); });
    }
}
