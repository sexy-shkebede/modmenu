#include "CAudioSource.hpp"
#include "../il2cpp/CClass.hpp"
#include "../il2cpp/CObject.hpp"
#include "../mono/CType.hpp"
#include "../mono/SString.hpp"
#include "../mono/SArray.hpp"
#include "../sdk_core.hpp"

namespace usdk
{
    il2cpp::CClass* CAudioSource::GetIl2cppClass()
    {
        RESOLVE_SDK_IL2CPP_CLASS("AudioSource", "UnityEngine", "UnityEngine.AudioModule.dll");
    }

    mono::CType* CAudioSource::GetMonoType()
    {
        RESOLVE_SDK_MONO_TYPE("UnityEngine.AudioSource, UnityEngine.AudioModule.dll");
    }

    void CAudioSource::Play()
    {
        RESOLVE_SDK_METHOD(void (*)( CAudioSource* ), this->GetIl2cppClass(), "Play", 0);
        func(this);
    }

    void CAudioSource::Stop()
    {
        RESOLVE_SDK_METHOD(void (*)( CAudioSource* ), this->GetIl2cppClass(), "Stop", 0);
        func(this);
    }

    void CAudioSource::Pause()
    {
        RESOLVE_SDK_METHOD(void (*)( CAudioSource* ), this->GetIl2cppClass(), "Pause", 0);
        func(this);
    }

    void CAudioSource::UnPause()
    {
        RESOLVE_SDK_METHOD(void (*)( CAudioSource* ), this->GetIl2cppClass(), "UnPause", 0);
        func(this);
    }

    float CAudioSource::GetVolume()
    {
        RESOLVE_SDK_METHOD(float (*)( CAudioSource* ), this->GetIl2cppClass(), "get_volume", 0);
        return func(this);
    }

    void CAudioSource::SetVolume(float fVolume)
    {
        RESOLVE_SDK_METHOD(void (*)( CAudioSource*, float ), this->GetIl2cppClass(), "set_volume", 1);
        func(this, fVolume);
    }

    float CAudioSource::GetPitch()
    {
        RESOLVE_SDK_METHOD(float (*)( CAudioSource* ), this->GetIl2cppClass(), "get_pitch", 0);
        return func(this);
    }

    void CAudioSource::SetPitch(float fPitch)
    {
        RESOLVE_SDK_METHOD(void (*)( CAudioSource*, float ), this->GetIl2cppClass(), "set_pitch", 1);
        func(this, fPitch);
    }

    bool CAudioSource::GetLoop()
    {
        RESOLVE_SDK_METHOD(bool (*)( CAudioSource* ), this->GetIl2cppClass(), "get_loop", 0);
        return func(this);
    }

    void CAudioSource::SetLoop(bool bState)
    {
        RESOLVE_SDK_METHOD(void (*)( CAudioSource*, bool ), this->GetIl2cppClass(), "set_loop", 1);
        func(this, bState);
    }

    bool CAudioSource::GetIsPlaying()
    {
        RESOLVE_SDK_METHOD(bool (*)( CAudioSource* ), this->GetIl2cppClass(), "get_isPlaying", 0);
        return func(this);
    }

    CAudioClip* CAudioSource::GetClip()
    {
        RESOLVE_SDK_METHOD(CAudioClip* (*)( CAudioSource* ), this->GetIl2cppClass(), "get_clip", 0);
        return func(this);
    }

    void CAudioSource::SetClip(CAudioClip* clip)
    {
        RESOLVE_SDK_METHOD(void (*)( CAudioSource*, CAudioClip* ), this->GetIl2cppClass(), "set_clip", 1);
        func(this, clip);
    }
} // usdk
