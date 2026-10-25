cmake_minimum_required(VERSION 3.22.1)
project(OpenXRHookTest)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ---- Dobby: статической либой внутрь нашего .so. Плагин SymbolResolver не используется ----
set(DOBBY_GENERATE_SHARED OFF CACHE BOOL "" FORCE)
set(Plugin.SymbolResolver OFF CACHE BOOL "" FORCE)
add_subdirectory(extern/dobby)
target_compile_options(dobby PRIVATE -O2)   # C++-часть Dobby иначе собиралась без оптимизаций

# Имя библиотеки прежнее: smali-патч грузит именно "openxrhooktest".
# ---- Unity-API: обёртки IL2CPP / Unity (photon/ не собирается: в нём свои ошибки include) ----
file(GLOB UNITY_API_SOURCES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/extern/unity-api/il2cpp/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/extern/unity-api/mono/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/extern/unity-api/unity/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/extern/unity-api/unity/common/*.cpp)

add_library(openxrhooktest SHARED
    native-lib.cpp

    menu/keyboard.cpp
    menu/menu.cpp
    menu/pages.cpp
    menu/settings.cpp
    menu/theme.cpp
    menu/ui.cpp

    fly/connectome.cpp
    fly/brain.cpp
    fly/sim.cpp
    fly/body.cpp
    fly/draw.cpp
    fly/brainview.cpp

    mods/mods.cpp
    mods/fly.cpp
    mods/il2cpp_util.cpp
    mods/unity_bridge.cpp

    openxr_hook/openxr_hook.cpp
    openxr_hook/xr_input.cpp
    openxr_hook/xr_composition.cpp
    openxr_hook/renderer/openxr_renderer.cpp
    openxr_hook/renderer/vulkan_backend.cpp
    openxr_hook/renderer/opengl_backend.cpp

    extern/imgui/imgui.cpp
    extern/imgui/imgui_draw.cpp
    extern/imgui/imgui_widgets.cpp
    extern/imgui/imgui_tables.cpp
    extern/imgui/backends/imgui_impl_vulkan.cpp
    extern/imgui/backends/imgui_impl_opengl3.cpp

    ${UNITY_API_SOURCES})

target_include_directories(openxrhooktest PRIVATE
    .
    openxr_hook
    extern/imgui
    extern/imgui/backends
    extern/dobby/include
    extern/unity-api
)

# Настройки ImGui (без debug-инструментов, assert пишет в logcat вместо падения игры)
target_compile_definitions(openxrhooktest PRIVATE IMGUI_USER_CONFIG="imgui_config.hpp")

# -O2 даже в Debug-сборке: по умолчанию там -O0, и ImGui работает в разы медленнее.
# Скрытые символы: наружу торчит только JNI_OnLoad, а наши ImGui и Dobby не конфликтуют
# с такими же библиотеками внутри игры или других модов.
target_compile_options(openxrhooktest PRIVATE -O2 -fvisibility=hidden -fvisibility-inlines-hidden)
target_link_options(openxrhooktest PRIVATE -Wl,--gc-sections -Wl,--exclude-libs,ALL)

# z — распаковка fly_brain.bin (zlib есть в любой прошивке Android)
target_link_libraries(openxrhooktest log vulkan EGL GLESv3 z dobby)
