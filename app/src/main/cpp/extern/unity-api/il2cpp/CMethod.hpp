#pragma once
#include <cstdint>
#include "../sdk_il2cpp_offsets.hpp"
#include <cstddef>

namespace il2cpp
{
    class CClass;
    class CType;

    class CMethod
    {
    public:
        inline void*       GetMethodPointer()        { return *(void**)((uintptr_t)this       + SDK_IL2CPP_OFFSET_METHOD_PTR);         }
        inline void*       GetVirtualMethodPointer() { return *(void**)((uintptr_t)this       + SDK_IL2CPP_OFFSET_METHOD_VIRTUAL_PTR); }
        inline void*       GetInvokerMethod()        { return *(void**)((uintptr_t)this       + SDK_IL2CPP_OFFSET_METHOD_INVOKER);     }
        inline const char* GetName()                 { return *(const char**)((uintptr_t)this + SDK_IL2CPP_OFFSET_METHOD_NAME);        }
        inline CClass*     GetClass()                { return *(CClass**)((uintptr_t)this     + SDK_IL2CPP_OFFSET_METHOD_CLASS);       }
        inline CType*      GetReturnType()           { return *(CType**)((uintptr_t)this      + SDK_IL2CPP_OFFSET_METHOD_RETURN_TYPE); }
        inline CType**     GetArgs()                 { return *(CType***)((uintptr_t)this     + SDK_IL2CPP_OFFSET_METHOD_ARGS);        }
        inline uint8_t     GetArgCount()             { return *(uint8_t*)((uintptr_t)this     + SDK_IL2CPP_OFFSET_METHOD_ARG_COUNT);   }

        inline void Invoke(void* pInstance, void** ppArgs, void* pRet)
        {
            if (auto invoker = reinterpret_cast<void(*)(void*, CMethod*, void*, void**, void*)>(GetInvokerMethod()))
                invoker(GetMethodPointer(), this, pInstance, ppArgs, pRet);
        }

        template <typename... TArgs>
        inline void Invoke(void* pInstance, void* pRet, TArgs... args)
        {
            constexpr size_t numArgs = sizeof...(TArgs);
            if constexpr (numArgs > 0)
            {
                void* ppArgs[numArgs] = { (void*)args... };
                Invoke(pInstance, ppArgs, pRet);
            }
            else
            {
                Invoke(pInstance, nullptr, pRet);
            }
        }

        template <typename TRet, typename... TArgs>
        inline TRet InvokeViaPtr(void* pInstance, TArgs... args)
        {
            auto pFunc = reinterpret_cast<TRet (*)(void*, TArgs...)>(GetMethodPointer());
            return pFunc(pInstance, args...);
        }
    };
} //il2cpp
