#pragma once

#include "CObject.hpp"

#include "../sdk_fwd.hpp"

namespace usdk
{
    class CAudioClip : public CObject
    {
    public:
        static il2cpp::CClass* GetIl2cppClass();
        static mono::CType*    GetMonoType();

        float GetLength();
        int   GetSamples();
        int   GetChannels();
        int   GetFrequency();

        bool  LoadAudioData();
        bool  GetData(mono::SArray<float>* pData, int iOffsetSamples);
        bool  SetData(mono::SArray<float>* pData, int iOffsetSamples);

        static CAudioClip* Create(const char* pszName, int iLengthSamples, int iChannels, int iFrequency, bool bStream);
    };
} // usdk
