#pragma once
// Подключается внутрь imgui.h через IMGUI_USER_CONFIG (см. CMakeLists.txt).

#include <android/log.h>

// Отладочные окна и логи ImGui в меню не используются — вырезаем из сборки.
#define IMGUI_DISABLE_DEBUG_TOOLS
#define IMGUI_DISABLE_OBSOLETE_FUNCTIONS

// 32-битные индексы: слой мира с блоками легко набирает больше 65 535 вершин, а бэкенд
// GLES 3.0 не умеет делить такой список (VtxOffset есть только с GL 3.2).
#define ImDrawIdx unsigned int

// Сработавший assert ImGui не должен ронять чужую игру целиком: пишем в logcat и живём дальше.
#define IM_ASSERT(_EXPR) do { if (!(_EXPR)) __android_log_print(ANDROID_LOG_ERROR, "shkbd", "ImGui assert: %s (%s:%d)", #_EXPR, __FILE__, __LINE__); } while (0)
