#pragma once

#include <stddef.h>
#include <stdint.h>
#include <string>

// Минимальный доступ к IL2CPP через экспорты libil2cpp.so, с проверкой каждого шага.
// Вызывать только из главного потока игры (UnityMain): он уже подключён к рантайму.
namespace il2
{
    bool Init();   // false — libil2cpp.so или нужные экспорты недоступны

    // Метод, вызываемый напрямую по нативному указателю. Скомпилированный IL2CPP-метод
    // последним аргументом принимает свой MethodInfo* — передаём честно.
    struct SMethod
    {
        const void* pInfo = nullptr;
        void*       pFunc = nullptr;

        explicit operator bool() const { return pFunc != nullptr; }

        template <typename TRet, typename... TArgs>
        TRet Call(TArgs... args) const
        {
            return reinterpret_cast<TRet (*)(TArgs..., const void*)>(pFunc)(args..., pInfo);
        }
    };

    void*   FindClass(const char* pszNamespace, const char* pszName);   // во всех загруженных сборках
    // Метод по имени и числу аргументов, с поиском по родительским классам.
    // Generic-определения (без кода) пропускаются.
    SMethod Find(void* pClass, const char* pszName, int nArgs, bool bLogMissing = true);
    // Перегрузка по точным типам параметров (имена IL: "UnityEngine.Vector3", "System.Single",
    // "UnityEngine.RaycastHit&" для out/ref). Только в самом классе.
    SMethod FindByParams(void* pClass, const char* pszName, const char* const* apszTypes, int nArgs);

    void*       ObjectClass(void* pObj);
    void*       TypeObject(void* pClass);                            // System.Type
    void*       NewObject(void* pClass);                             // без вызова конструктора
    void*       NewString(const char* pszUtf8);
    std::string ToUtf8(const void* pString);
    bool        GetField(void* pObj, const char* pszName, void* pOut);   // поле экземпляра (и родителей)
    void*       NewArray(void* pElementClass, size_t n);
    size_t      ArrayLength(const void* pArray);
    void* const* ArrayData(const void* pArray);                      // массив ссылок

    // GC-handle держит управляемую обёртку живой между кадрами. Если игра уничтожит сам объект,
    // обёртка останется, а проверка Object.op_Implicit вернёт false — без висячих указателей.
    uint32_t NewGCHandle(void* pObj);    // 0 — не вышло
    void*    GCHandleTarget(uint32_t h);
    void     FreeGCHandle(uint32_t h);
}
