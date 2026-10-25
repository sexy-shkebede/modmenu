// fly_brain.bin внутри библиотеки. Собирается, только если CMake нашёл файл (FLY_BRAIN_BIN):
// он передаёт путь в FLY_BRAIN_PATH, и ассемблер вставляет файл как есть (.incbin) в .rodata.
// Выравнивание 16 — данные без сжатия читаются прямо отсюда (SConnectome::LoadMemory).
#include "brain_blob.hpp"

#ifdef FLY_BRAIN_PATH
__asm__(
    "    .pushsection .rodata.fly_brain, \"a\"\n"
    "    .balign 16\n"
    "    .global g_FlyBrainBin\n"
    "    .hidden g_FlyBrainBin\n"
    "g_FlyBrainBin:\n"
    "    .incbin \"" FLY_BRAIN_PATH "\"\n"
    "    .global g_FlyBrainBinEnd\n"
    "    .hidden g_FlyBrainBinEnd\n"
    "g_FlyBrainBinEnd:\n"
    "    .popsection\n");
#endif
