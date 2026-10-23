#pragma once

#include <cstdint>

// settings
//#define SDK_STRING_OBFUSCATION  // Encrypts strings using oxorany
#define SDK_SAFE_CALLS            // Null-checks pClass/pMethod/pInfo before use. Disable for max speed
#define SDK_SAFE_CALLS_LOGGING    // Logs whether a method/class is found or not

//  cross-platform helpers

#define SDK_PAD_PTRS(name, count) char name[sizeof(void*) * (count)]

#if defined(_MSC_VER)
    #define SDK_FORCEINLINE __forceinline
#elif defined(__GNUC__) || defined(__clang__)
    #define SDK_FORCEINLINE __attribute__((always_inline)) inline
#else
    #define SDK_FORCEINLINE inline
#endif

#if defined(__clang__) || defined(__GNUC__)
    #define SDK_LIKELY(x)   __builtin_expect(!!(x), 1)
    #define SDK_UNLIKELY(x) __builtin_expect(!!(x), 0)
#else
    #define SDK_LIKELY(x)   (x)
    #define SDK_UNLIKELY(x) (x)
#endif

#if defined(_MSC_VER)
    #define SDK_ASSUME(x) __assume(x)
#elif defined(__clang__)
    #define SDK_ASSUME(x) __builtin_assume(x)
#elif defined(__GNUC__)
    #define SDK_ASSUME(x) do { if (!(x)) __builtin_unreachable(); } while(0)
#else
    #define SDK_ASSUME(x) (void)(x)
#endif

// obfuscation
#ifdef SDK_STRING_OBFUSCATION
    #include "../../../external/oxorany/include.h"
    #define OBFUSCATE_STR(str) oxorany(str)
#else
    #define OBFUSCATE_STR(str) str
#endif

#ifdef SDK_SAFE_CALLS_LOGGING
    #ifdef __ANDROID__
        #include <android/log.h>
        #define SDK_PRINTF(...) __android_log_print(ANDROID_LOG_INFO, OBFUSCATE_STR("Unity-SDK"), __VA_ARGS__)
    #else
        #include <stdio.h>
        #define SDK_PRINTF(...) printf(__VA_ARGS__)
    #endif

    #define SDK_LOG(type, name, ptr)                                                                           \
        do {                                                                                                   \
            if (SDK_UNLIKELY(!(ptr))) SDK_PRINTF(OBFUSCATE_STR("[Unity-SDK] error: %s '%s' not found\n"), OBFUSCATE_STR(type), OBFUSCATE_STR(name)); \
            else                      SDK_PRINTF(OBFUSCATE_STR("[Unity-SDK] ok: %s '%s' -> 0x%p\n"), OBFUSCATE_STR(type), OBFUSCATE_STR(name), (void*)(ptr)); \
        } while(0)
#else
    #define SDK_LOG(type, name, ptr)
#endif

#ifdef SDK_SAFE_CALLS
    #define SDK_SAFE_RETURN()            if (SDK_UNLIKELY(!pMethod)) return;
    #define SDK_SAFE_RETURN_VAL(retVal)  if (SDK_UNLIKELY(!pMethod)) return retVal;
#else
    #define SDK_SAFE_RETURN()
    #define SDK_SAFE_RETURN_VAL(retVal)
#endif

// resolve macros

#define RESOLVE_SDK_IL2CPP_CLASS(name, name_space, dll)                                                                \
    static void* pClass = nullptr;                                                                                     \
    if (SDK_UNLIKELY(!pClass))                                                                                         \
    {                                                                                                                  \
        pClass = (void*)il2cpp::CClass::FromName(OBFUSCATE_STR(name), OBFUSCATE_STR(name_space), OBFUSCATE_STR(dll));  \
        SDK_LOG("Class", name, pClass);                                                                                \
    }                                                                                                                  \
    return (il2cpp::CClass*)pClass;

#define RESOLVE_SDK_MONO_TYPE(name)                                                                                    \
    static void* pType = nullptr;                                                                                      \
    if (SDK_UNLIKELY(!pType))                                                                                          \
    {                                                                                                                  \
        pType = (void*)mono::CType::FromName(OBFUSCATE_STR(name));                                                     \
        SDK_LOG("Type", name, pType);                                                                                  \
    }                                                                                                                  \
    return (mono::CType*)pType;

#ifdef SDK_SAFE_CALLS
#define RESOLVE_SDK_METHOD(funcType, classPtr, methodName, argCount)                                                   \
    static void* pMethod = nullptr;                                                                                    \
    if (SDK_UNLIKELY(!pMethod))                                                                                        \
    {                                                                                                                  \
        if (auto pClass = (classPtr))                                                                                  \
            if (auto pInfo = pClass->GetMethodFromName(OBFUSCATE_STR(methodName), argCount))                           \
                pMethod = (void*)pInfo->GetMethodPointer();                                                            \
        SDK_LOG("Method", methodName, pMethod);                                                                        \
    }                                                                                                                  \
    auto func = (funcType)pMethod;
#else
#define RESOLVE_SDK_METHOD(funcType, classPtr, methodName, argCount)                                                   \
    static void* pMethod = nullptr;                                                                                    \
    if (SDK_UNLIKELY(!pMethod))                                                                                        \
    {                                                                                                                  \
        pMethod = (void*)(classPtr)->GetMethodFromName(OBFUSCATE_STR(methodName), argCount)->GetMethodPointer();       \
        SDK_LOG("Method", methodName, pMethod);                                                                        \
    }                                                                                                                  \
    SDK_ASSUME(pMethod != nullptr);                                                                                    \
    auto func = (funcType)pMethod;
#endif
