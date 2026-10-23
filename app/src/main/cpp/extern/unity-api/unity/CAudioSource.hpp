#pragma once

#include "CAudioClip.hpp"
#include "CComponent.hpp"

#include "../sdk_fwd.hpp"

namespace usdk
{
    class CAudioSource : public CComponent
    {
    public:
        static il2cpp::CClass* GetIl2cppClass();
        static mono::CType*    GetMonoType();
        
        void Play();
        void Stop();
        void Pause();
        void UnPause();

        float GetVolume();
        void SetVolume(float fVolume);

        float GetPitch();
        void SetPitch(float fPitch);

        bool GetLoop();
        void SetLoop(bool bState);

        bool GetIsPlaying();

        CAudioClip* GetClip();
        void SetClip(CAudioClip* pClip);
    };
} // usdk
