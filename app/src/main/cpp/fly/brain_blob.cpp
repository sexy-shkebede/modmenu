// fly_brain.bin и fly_vnc.bin внутри библиотеки. Собирается, только если CMake нашёл файлы
// (FLY_BRAIN_BIN, FLY_VNC_BIN): он передаёт пути в FLY_BRAIN_PATH / FLY_VNC_PATH, и ассемблер
// вставляет файлы как есть (.incbin) в .rodata. Выравнивание 16 — данные без сжатия читаются
// прямо отсюда (SConnectome::LoadMemory, SVncData::LoadMemory).
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

#ifdef FLY_VNC_PATH
__asm__(
    "    .pushsection .rodata.fly_vnc, \"a\"\n"
    "    .balign 16\n"
    "    .global g_FlyVncBin\n"
    "    .hidden g_FlyVncBin\n"
    "g_FlyVncBin:\n"
    "    .incbin \"" FLY_VNC_PATH "\"\n"
    "    .global g_FlyVncBinEnd\n"
    "    .hidden g_FlyVncBinEnd\n"
    "g_FlyVncBinEnd:\n"
    "    .popsection\n");
#endif
