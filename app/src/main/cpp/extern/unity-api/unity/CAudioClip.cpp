#include "CAudioClip.hpp"
#include "../il2cpp/CClass.hpp"
#include "../il2cpp/CObject.hpp"
#include "../mono/CType.hpp"
#include "../mono/SString.hpp"
#include "../mono/SArray.hpp"
#include "../sdk_core.hpp"

namespace usdk
{
    il2cpp::CClass* CAudioClip::GetIl2cppClass()
    {
        RESOLVE_SDK_IL2CPP_CLASS("AudioClip", "UnityEngine", "UnityEngine.AudioModule.dll");
    }

    mono::CType* CAudioClip::GetMonoType()
    {
        RESOLVE_SDK_MONO_TYPE("UnityEngine.AudioClip, UnityEngine.AudioModule.dll");
    }

    float CAudioClip::GetLength()
    {
        RESOLVE_SDK_METHOD(float (*)( CAudioClip* ), this->GetIl2cppClass(), "get_length", 0);
        return func(this);
    }

    int CAudioClip::GetSamples()
    {
        RESOLVE_SDK_METHOD(int (*)( CAudioClip* ), this->GetIl2cppClass(), "get_samples", 0);
        return func(this);
    }

    int CAudioClip::GetChannels()
    {
        RESOLVE_SDK_METHOD(int (*)( CAudioClip* ), this->GetIl2cppClass(), "get_channels", 0);
        return func(this);
    }

    int CAudioClip::GetFrequency()
    {
        RESOLVE_SDK_METHOD(int (*)( CAudioClip* ), this->GetIl2cppClass(), "get_frequency", 0);
        return func(this);
    }

    bool CAudioClip::LoadAudioData()
    {
        RESOLVE_SDK_METHOD(bool (*)( CAudioClip* ), this->GetIl2cppClass(), "LoadAudioData", 0);
        return func(this);
    }

    bool CAudioClip::GetData(mono::SArray<float>* pData, int iOffsetSamples)
    {
        RESOLVE_SDK_METHOD(bool (*)( CAudioClip*, mono::SArray<float>*, int ), this->GetIl2cppClass(), "GetData", 2);
        return func(this, pData, iOffsetSamples);
    }

    bool CAudioClip::SetData(mono::SArray<float>* pData, int iOffsetSamples)
    {
        RESOLVE_SDK_METHOD(bool (*)( CAudioClip*, mono::SArray<float>*, int ), this->GetIl2cppClass(), "SetData", 2);
        return func(this, pData, iOffsetSamples);
    }

    CAudioClip* CAudioClip::Create(const char* pszName, int iLengthSamples, int iChannels, int iFrequency, bool bStream)
    {
        RESOLVE_SDK_METHOD(CAudioClip* (*)( mono::SString*, int, int, int, bool ), GetIl2cppClass(), "Create", 5);
        return func(mono::SString::Create(pszName), iLengthSamples, iChannels, iFrequency, bStream);
    }
} // usdk
