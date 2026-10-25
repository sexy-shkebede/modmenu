// shkbd internal — точка входа: ставим хук на OpenXR-загрузчик игры и отдаём ему меню.

#include <jni.h>
#include <dlfcn.h>
#include <time.h>
#include <pthread.h>
#include <android/log.h>

#include "openxr_hook/openxr_hook.hpp"
#include "menu/menu.hpp"
#include "menu/settings.hpp"
#include "mods/mods.hpp"
#include "mods/fly.hpp"

#define LOG_TAG "shkbd"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO,  LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

static COpenXRHook g_Hook;

static void SleepMs(long ms)
{
    timespec ts = { ms / 1000, (ms % 1000) * 1000000L };
    nanosleep(&ts, nullptr);
}

static void* InitThread(void*)
{
    // Лучший вариант: подгружаем OpenXR-загрузчик игры сами (он лежит в lib/ того же APK).
    // Тогда хук встаёт раньше, чем игра успевает запросить у OpenXR хоть одну функцию,
    // и гонки "хук не успел встать" нет в принципе.
    void* hLoader = dlopen("libopenxr_loader.so", RTLD_NOW);

    // Запасной вариант: по имени не нашёлся — ждём, пока игра загрузит его сама.
    // Первые ~10 с проверяем каждые 2 мс, дальше раз в 50 мс (в играх без OpenXR поток почти не ест CPU).
    for (int i = 0; !hLoader; i++)
    {
        SleepMs(i < 5000 ? 2 : 50);
        hLoader = dlopen("libopenxr_loader.so", RTLD_NOW | RTLD_NOLOAD);
    }

    // Хендл не закрываем: загрузчик не должен выгрузиться, пока на нём стоит хук.
    g_Hook.m_pfnSetupMenu      = menu::Setup;
    g_Hook.m_pfnDrawMenu       = menu::Draw;
    g_Hook.m_pfnMainThreadTick = mods::Tick;            // всё, что трогает объекты Unity
    g_Hook.m_pfnMenuScale      = settings::PanelScale;
    g_Hook.m_pfnDrawSprites    = menu::DrawSprites;
    g_Hook.m_pfnSetupWorld     = flymod::SetupWorld;     // муха и сахар — слоем мира
    g_Hook.m_pfnWorldActive    = flymod::WorldActive;
    g_Hook.m_pfnDrawWorld      = flymod::DrawWorld;

    if (g_Hook.Initialize(hLoader)) LOGI("OpenXR hook installed");
    else                            LOGE("OpenXR hook FAILED: xrGetInstanceProcAddr not found or DobbyHook error");
    return nullptr;
}

extern "C" JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM*, void*)
{
    pthread_t thread;
    if (pthread_create(&thread, nullptr, InitThread, nullptr) == 0)
        pthread_detach(thread);
    return JNI_VERSION_1_6;
}
