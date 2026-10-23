#pragma once

#include "CComponent.hpp"
#include "CAudioClip.hpp"

#include "sdk_fwd.hpp"

namespace photonsdk
{
    class CRecorder : public usdk::CComponent 
    {
    public:
        static il2cpp::CClass* GetIl2cppClass();
        static mono::CType*    GetMonoType();

        enum class EInputSourceType : int 
        {
            eMicrophone = 0,
            eAudioClip = 1
        };

        EInputSourceType    GetSourceType();
        void                SetSourceType(EInputSourceType value);
        bool                GetDebugEchoMode();
        void                SetDebugEchoMode(bool value);
        usdk::CAudioClip*   GetAudioClip();
        void                SetAudioClip(usdk::CAudioClip *clip);
        void                SetTransmitEnabled(bool bValue);
        void                RestartRecording();
        void                RestartRecording(bool bForce);
    };
}
