#include "il2cpp_util.hpp"

#include <dlfcn.h>
#include <stdint.h>
#include <string.h>
#include <type_traits>
#include <android/log.h>

#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, "shkbd", __VA_ARGS__)

namespace
{
    struct SApi
    {
        void*        (*domain_get)();
        void**       (*domain_get_assemblies)(const void*, size_t*);
        void*        (*assembly_get_image)(const void*);
        void*        (*class_from_name)(const void*, const char*, const char*);
        void*        (*class_get_methods)(void*, void**);
        void*        (*class_get_parent)(void*);
        void*        (*class_get_field_from_name)(void*, const char*);
        const void*  (*class_get_type)(void*);
        const char*  (*method_get_name)(const void*);
        uint32_t     (*method_get_param_count)(const void*);
        void         (*field_get_value)(void*, void*, void*);
        void*        (*object_get_class)(void*);
        void*        (*object_new)(void*);
        void*        (*string_new)(const char*);
        void*        (*type_get_object)(const void*);
        void*        (*array_new)(void*, uintptr_t);
        // необязательные: нужны только для поиска перегрузок по типам
        const void*  (*method_get_param)(const void*, uint32_t);
        char*        (*type_get_name)(const void*);
        void         (*free_)(void*);
        uint32_t     (*gchandle_new)(void*, bool);
        void*        (*gchandle_get_target)(uint32_t);
        void         (*gchandle_free)(uint32_t);
    } A {};
    int g_iState = 0;   // 0 — не пробовали, 1 — готово, -1 — недоступно

    // Il2CppString: заголовок объекта (16 байт), int32 length, затем UTF-16
    constexpr size_t kStringLength = 0x10, kStringChars = 0x14;
    // Il2CppArray: заголовок (16), bounds (8), max_length (8), затем данные
    constexpr size_t kArrayLength = 0x18, kArrayData = 0x20;
}

bool il2::Init()
{
    if (g_iState != 0) return g_iState > 0;
    g_iState = -1;

    void* h = dlopen("libil2cpp.so", RTLD_NOW | RTLD_NOLOAD);
    if (!h) { LOGW("il2cpp: libil2cpp.so is not loaded"); return false; }

    bool bOk = true;
    const auto Sym = [&](auto& pfn, const char* pszName)
    {
        pfn = reinterpret_cast<std::remove_reference_t<decltype(pfn)>>(dlsym(h, pszName));
        if (!pfn) { LOGW("il2cpp: export %s is missing", pszName); bOk = false; }
    };
    Sym(A.domain_get,                "il2cpp_domain_get");
    Sym(A.domain_get_assemblies,     "il2cpp_domain_get_assemblies");
    Sym(A.assembly_get_image,        "il2cpp_assembly_get_image");
    Sym(A.class_from_name,           "il2cpp_class_from_name");
    Sym(A.class_get_methods,         "il2cpp_class_get_methods");
    Sym(A.class_get_parent,          "il2cpp_class_get_parent");
    Sym(A.class_get_field_from_name, "il2cpp_class_get_field_from_name");
    Sym(A.class_get_type,            "il2cpp_class_get_type");
    Sym(A.method_get_name,           "il2cpp_method_get_name");
    Sym(A.method_get_param_count,    "il2cpp_method_get_param_count");
    Sym(A.field_get_value,           "il2cpp_field_get_value");
    Sym(A.object_get_class,          "il2cpp_object_get_class");
    Sym(A.object_new,                "il2cpp_object_new");
    Sym(A.string_new,                "il2cpp_string_new");
    Sym(A.type_get_object,           "il2cpp_type_get_object");
    Sym(A.array_new,                 "il2cpp_array_new");
    A.method_get_param = reinterpret_cast<decltype(A.method_get_param)>(dlsym(h, "il2cpp_method_get_param"));
    A.type_get_name    = reinterpret_cast<decltype(A.type_get_name)>(dlsym(h, "il2cpp_type_get_name"));
    A.free_            = reinterpret_cast<decltype(A.free_)>(dlsym(h, "il2cpp_free"));
    A.gchandle_new        = reinterpret_cast<decltype(A.gchandle_new)>(dlsym(h, "il2cpp_gchandle_new"));
    A.gchandle_get_target = reinterpret_cast<decltype(A.gchandle_get_target)>(dlsym(h, "il2cpp_gchandle_get_target"));
    A.gchandle_free       = reinterpret_cast<decltype(A.gchandle_free)>(dlsym(h, "il2cpp_gchandle_free"));
    g_iState = bOk ? 1 : -1;
    return bOk;
}

void* il2::FindClass(const char* pszNamespace, const char* pszName)
{
    if (!Init()) return nullptr;
    size_t n = 0;
    void** ppAssemblies = A.domain_get_assemblies(A.domain_get(), &n);
    for (size_t i = 0; ppAssemblies && i < n; i++)
        if (void* pImage = A.assembly_get_image(ppAssemblies[i]))
            if (void* pClass = A.class_from_name(pImage, pszNamespace, pszName))
                return pClass;
    return nullptr;
}

il2::SMethod il2::Find(void* pClass, const char* pszName, int nArgs, bool bLogMissing)
{
    SMethod m;
    if (Init())
        for (void* k = pClass; k && !m; k = A.class_get_parent(k))
        {
            void* iter = nullptr;
            while (void* p = A.class_get_methods(k, &iter))
            {
                if ((int)A.method_get_param_count(p) != nArgs) continue;
                void* pFunc = *reinterpret_cast<void* const*>(p);   // MethodInfo::methodPointer
                if (!pFunc) continue;
                const char* psz = A.method_get_name(p);
                if (!psz || strcmp(psz, pszName) != 0) continue;
                m.pInfo = p;
                m.pFunc = pFunc;
                break;
            }
        }
    if (!m && bLogMissing && pClass) LOGW("il2cpp: method %s (%d args) not found", pszName, nArgs);
    return m;
}

il2::SMethod il2::FindByParams(void* pClass, const char* pszName, const char* const* apszTypes, int nArgs)
{
    SMethod m;
    if (!pClass || !Init() || !A.method_get_param || !A.type_get_name) return m;
    void* iter = nullptr;
    while (void* p = A.class_get_methods(pClass, &iter))
    {
        if ((int)A.method_get_param_count(p) != nArgs) continue;
        void*       pFunc = *reinterpret_cast<void* const*>(p);
        const char* psz   = A.method_get_name(p);
        if (!pFunc || !psz || strcmp(psz, pszName) != 0) continue;
        bool bMatch = true;
        for (int i = 0; i < nArgs && bMatch; i++)
        {
            char* pszType = A.type_get_name(A.method_get_param(p, (uint32_t)i));
            bMatch = pszType && strcmp(pszType, apszTypes[i]) == 0;
            if (pszType && A.free_) A.free_(pszType);
        }
        if (bMatch) { m.pInfo = p; m.pFunc = pFunc; break; }
    }
    return m;
}

uint32_t il2::NewGCHandle(void* pObj)     { return pObj && Init() && A.gchandle_new ? A.gchandle_new(pObj, false) : 0; }
void*    il2::GCHandleTarget(uint32_t h)  { return h && Init() && A.gchandle_get_target ? A.gchandle_get_target(h) : nullptr; }
void     il2::FreeGCHandle(uint32_t h)    { if (h && Init() && A.gchandle_free) A.gchandle_free(h); }

void* il2::ObjectClass(void* pObj)   { return pObj && Init() ? A.object_get_class(pObj) : nullptr; }
void* il2::TypeObject(void* pClass)  { return pClass && Init() ? A.type_get_object(A.class_get_type(pClass)) : nullptr; }
void* il2::NewObject(void* pClass)   { return pClass && Init() ? A.object_new(pClass) : nullptr; }
void* il2::NewString(const char* s)  { return Init() ? A.string_new(s ? s : "") : nullptr; }
void* il2::NewArray(void* pClass, size_t n) { return pClass && Init() ? A.array_new(pClass, n) : nullptr; }

size_t il2::ArrayLength(const void* pArray)
{
    return pArray ? (size_t)*reinterpret_cast<const uintptr_t*>((const char*)pArray + kArrayLength) : 0;
}

void* const* il2::ArrayData(const void* pArray)
{
    return reinterpret_cast<void* const*>((const char*)pArray + kArrayData);
}

bool il2::GetField(void* pObj, const char* pszName, void* pOut)
{
    if (!pObj || !Init()) return false;
    for (void* k = A.object_get_class(pObj); k; k = A.class_get_parent(k))
        if (void* pField = A.class_get_field_from_name(k, pszName))
        {
            A.field_get_value(pObj, pField, pOut);
            return true;
        }
    return false;
}

std::string il2::ToUtf8(const void* pString)
{
    std::string out;
    if (!pString) return out;
    const int32_t   len = *reinterpret_cast<const int32_t*>((const char*)pString + kStringLength);
    const char16_t* p   = reinterpret_cast<const char16_t*>((const char*)pString + kStringChars);
    for (int32_t i = 0; i < len && i < 4096; i++)
    {
        uint32_t c = p[i];
        if (c >= 0xD800 && c <= 0xDBFF && i + 1 < len && p[i + 1] >= 0xDC00 && p[i + 1] <= 0xDFFF)
            c = 0x10000 + ((c - 0xD800) << 10) + (p[++i] - 0xDC00);
        if (c < 0x80)        out += (char)c;
        else if (c < 0x800)  { out += (char)(0xC0 | (c >> 6));  out += (char)(0x80 | (c & 0x3F)); }
        else if (c < 0x10000){ out += (char)(0xE0 | (c >> 12)); out += (char)(0x80 | ((c >> 6) & 0x3F)); out += (char)(0x80 | (c & 0x3F)); }
        else                 { out += (char)(0xF0 | (c >> 18)); out += (char)(0x80 | ((c >> 12) & 0x3F)); out += (char)(0x80 | ((c >> 6) & 0x3F)); out += (char)(0x80 | (c & 0x3F)); }
    }
    return out;
}
