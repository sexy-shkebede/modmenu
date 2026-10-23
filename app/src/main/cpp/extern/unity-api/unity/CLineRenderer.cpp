#include "CLineRenderer.hpp"
#include "../il2cpp/CClass.hpp"
#include "../il2cpp/CObject.hpp"
#include "../mono/CType.hpp"
#include "../mono/SString.hpp"
#include "../mono/SArray.hpp"
#include "../sdk_core.hpp"

namespace usdk
{
    il2cpp::CClass* CLineRenderer::GetIl2cppClass()
    {
        RESOLVE_SDK_IL2CPP_CLASS("LineRenderer", "UnityEngine", "UnityEngine.CoreModule.dll");
    }

    mono::CType* CLineRenderer::GetMonoType()
    {
        RESOLVE_SDK_MONO_TYPE("UnityEngine.LineRenderer, UnityEngine.CoreModule.dll");
    }

    float CLineRenderer::GetWidthMultiplier()
    {
        RESOLVE_SDK_METHOD(float (*)( CLineRenderer* ), this->GetIl2cppClass(), "get_widthMultiplier", 0);
        return func(this);
    }

    void CLineRenderer::SetWidthMultiplier(float fValue)
    {
        RESOLVE_SDK_METHOD(void (*)( CLineRenderer*, float ), this->GetIl2cppClass(), "set_widthMultiplier", 1);
        func(this, fValue);
    }

    bool CLineRenderer::GetUseWorldSpace()
    {
        RESOLVE_SDK_METHOD(bool (*)( CLineRenderer* ), this->GetIl2cppClass(), "get_useWorldSpace", 0);
        return func(this);
    }

    void CLineRenderer::SetUseWorldSpace(bool bState)
    {
        RESOLVE_SDK_METHOD(void (*)( CLineRenderer*, bool ), this->GetIl2cppClass(), "set_useWorldSpace", 1);
        func(this, bState);
    }

    Color CLineRenderer::GetStartColor()
    {
        RESOLVE_SDK_METHOD(Color (*)( CLineRenderer* ), this->GetIl2cppClass(), "get_startColor", 0);
        return func(this);
    }

    void CLineRenderer::SetStartColor(Color color)
    {
        RESOLVE_SDK_METHOD(void (*)( CLineRenderer*, Color ), this->GetIl2cppClass(), "set_startColor", 1);
        func(this, color);
    }

    Color CLineRenderer::GetEndColor()
    {
        RESOLVE_SDK_METHOD(Color (*)( CLineRenderer* ), this->GetIl2cppClass(), "get_endColor", 0);
        return func(this);
    }

    void CLineRenderer::SetEndColor(Color color)
    {
        RESOLVE_SDK_METHOD(void (*)( CLineRenderer*, Color ), this->GetIl2cppClass(), "set_endColor", 1);
        func(this, color);
    }

    void CLineRenderer::SetPositionCount(int iValue)
    {
        RESOLVE_SDK_METHOD(void (*)( CLineRenderer*, int ), this->GetIl2cppClass(), "set_positionCount", 1);
        func(this, iValue);
    }

    void CLineRenderer::SetPosition(int iIndex, Vector3 vOrigin)
    {
        RESOLVE_SDK_METHOD(void (*)( CLineRenderer*, int, Vector3 ), this->GetIl2cppClass(), "SetPosition", 2);
        func(this, iIndex, vOrigin);
    }

    void CLineRenderer::SetPositions(mono::SArray<Vector3>* apvOrigins)
    {
        RESOLVE_SDK_METHOD(void (*)( CLineRenderer*, mono::SArray<Vector3>* ), this->GetIl2cppClass(), "SetPositions", 1);
        func(this, apvOrigins);
    }
} // usdk
