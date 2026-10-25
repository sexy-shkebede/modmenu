#pragma once

// Вшитый в библиотеку fly_brain.bin (fly/brain_blob.cpp). Есть, только если при сборке
// определён FLY_BRAIN_EMBEDDED (CMake нашёл файл).
#ifdef FLY_BRAIN_EMBEDDED
extern "C" const unsigned char g_FlyBrainBin[];
extern "C" const unsigned char g_FlyBrainBinEnd[];
#endif
