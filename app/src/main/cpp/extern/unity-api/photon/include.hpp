#pragma once

#define USE_PHOTON_FUSION
#define USE_PHOTON_VOICE

#ifdef USE_PHOTON_PUN
#include "pun/include.hpp"
#endif //USE_PHOTON_PUN

#ifdef USE_PHOTON_FUSION
#include "fusion/include.hpp"
#endif //USE_PHOTON_FUSION

#ifdef USE_PHOTON_VOICE
#include "voice/include.hpp"
#endif //USE_PHOTON_VOICE

#undef USE_PHOTON_PUN
#undef USE_PHOTON_FUSION
#undef USE_PHOTON_VOICE
