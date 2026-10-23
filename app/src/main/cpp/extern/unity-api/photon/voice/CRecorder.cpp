#include "CRecorder.hpp"
#include "../../il2cpp/CClass.hpp"
#include "../../il2cpp/CObject.hpp"
#include "../../mono/CType.hpp"
#include "../../mono/SString.hpp"
#include "../../mono/SArray.hpp"
#include "../../sdk_core.hpp"

namespace photonsdk
{
    il2cpp::CClass* CRecorder::GetIl2cppClass()
    {
        RESOLVE_SDK_IL2CPP_CLASS("Recorder", "Photon.Voice.Unity", "PhotonVoice.dll");
    }

    mono::CType* CRecorder::GetMonoType()
    {
        RESOLVE_SDK_MONO_TYPE("Photon.Voice.Unity.Recorder, PhotonVoice");
    }

    CRecorder::EInputSourceType CRecorder::GetSourceType()
    {
        RESOLVE_SDK_METHOD(EInputSourceType (*)( CRecorder * ), this->GetIl2cppClass(), "get_SourceType", 0);
        return func(this);
    }

    void CRecorder::SetSourceType(EInputSourceType value)
    {
        RESOLVE_SDK_METHOD(void (*)( CRecorder *, EInputSourceType ), this->GetIl2cppClass(), "set_SourceType", 1);
        func(this, value);
    }

    bool CRecorder::GetDebugEchoMode()
    {
        RESOLVE_SDK_METHOD(bool (*)( CRecorder * ), this->GetIl2cppClass(), "get_DebugEchoMode", 0);
        return func(this);
    }

    void CRecorder::SetDebugEchoMode(bool value)
    {
        RESOLVE_SDK_METHOD(void (*)( CRecorder *, bool ), this->GetIl2cppClass(), "set_DebugEchoMode", 1);
        func(this, value);
    }

    usdk::CAudioClip *CRecorder::GetAudioClip()
    {
        RESOLVE_SDK_METHOD(usdk::CAudioClip * (*)( CRecorder * ), this->GetIl2cppClass(), "get_AudioClip", 0);
        return func(this);
    }

    void CRecorder::SetAudioClip(usdk::CAudioClip *clip)
    {
        RESOLVE_SDK_METHOD(void (*)( CRecorder *, usdk::CAudioClip * ), this->GetIl2cppClass(), "set_AudioClip", 1);
        func(this, clip);
    }

    void CRecorder::SetTransmitEnabled(bool bValue)
    {
        RESOLVE_SDK_METHOD(void (*)( CRecorder*, bool ), this->GetIl2cppClass(), "set_TransmitEnabled", 1);
        func(this, bValue);
    }

    void CRecorder::RestartRecording()
    {
        RESOLVE_SDK_METHOD(void (*)( CRecorder* ), this->GetIl2cppClass(), "RestartRecording", 0);
        func(this);
    }

    void CRecorder::RestartRecording(bool bForce)
    {
        RESOLVE_SDK_METHOD(void (*)( CRecorder*, bool ), this->GetIl2cppClass(), "RestartRecording", 1);
        func(this, bForce);
    }
}
