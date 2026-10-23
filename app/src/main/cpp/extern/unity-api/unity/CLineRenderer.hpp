#pragma once

#include "CRenderer.hpp"
#include "common/include.hpp"

#include "../sdk_fwd.hpp"

namespace usdk
{
    class CLineRenderer : public CRenderer
    {
    public:
        static il2cpp::CClass* GetIl2cppClass();
        static mono::CType*    GetMonoType();

        float GetWidthMultiplier();
        void  SetWidthMultiplier(float fValue);
        bool  GetUseWorldSpace();
        void  SetUseWorldSpace(bool bState);
        Color GetStartColor();
        void  SetStartColor(Color color);
        Color GetEndColor();
        void  SetEndColor(Color color);
        void  SetPositionCount(int iValue);
        void  SetPosition(int iIndex, Vector3 vOrigin);
        void  SetPositions(mono::SArray<Vector3>* apvOrigins);
    };
} // usdk
