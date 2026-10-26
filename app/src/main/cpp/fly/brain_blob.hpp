#pragma once

// Вшитые в библиотеку fly_brain.bin и fly_vnc.bin (fly/brain_blob.cpp). Есть, только если при
// сборке определены FLY_BRAIN_EMBEDDED / FLY_VNC_EMBEDDED (CMake нашёл файл).
#ifdef FLY_BRAIN_EMBEDDED
extern "C" const unsigned char g_FlyBrainBin[];
extern "C" const unsigned char g_FlyBrainBinEnd[];
#endif
#ifdef FLY_VNC_EMBEDDED
extern "C" const unsigned char g_FlyVncBin[];
extern "C" const unsigned char g_FlyVncBinEnd[];
#endif
