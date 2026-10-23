#pragma once
// Подключается внутрь imgui.h через IMGUI_USER_CONFIG (см. CMakeLists.txt).

#include <android/log.h>

// Отладочные окна и логи ImGui в меню не используются — вырезаем из сборки.
#define IMGUI_DISABLE_DEBUG_TOOLS
#define IMGUI_DISABLE_OBSOLETE_FUNCTIONS

// Сработавший assert ImGui не должен ронять чужую игру целиком: пишем в logcat и живём дальше.
#define IM_ASSERT(_EXPR) do { if (!(_EXPR)) __android_log_print(ANDROID_LOG_ERROR, "shkbd", "ImGui assert: %s (%s:%d)", #_EXPR, __FILE__, __LINE__); } while (0)
