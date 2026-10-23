#include "CIl2cppApi.hpp"
#include "../sdk_core.hpp"
#include "../sdk_il2cpp_exports.hpp"

#ifdef __ANDROID__
#include <dlfcn.h>
#else
#include <windows.h>
#endif

namespace il2cpp
{
    CIl2cppApi Api;

    bool CIl2cppApi::Initialize(void* pIl2cppHandle)
    { 
        #define API_GET_EXPORT_PTR(method, methodPtr)  method = (decltype(method))((uintptr_t)pIl2cppHandle + methodPtr);
        #ifdef __ANDROID__
        #define API_GET_EXPORT_SYM(method, methodName) method = (decltype(method))dlsym(pIl2cppHandle, methodName);
        #else
        #define API_GET_EXPORT_SYM(method, methodName) method = (decltype(method))GetProcAddress(pIl2cppHandle, methodName);
        #endif

        API_GET_EXPORT_SYM(this->array_new,                       OBFUSCATE_STR(SDK_IL2CPP_EXPORT_SYM_array_new));
        API_GET_EXPORT_SYM(this->class_from_name,                 OBFUSCATE_STR(SDK_IL2CPP_EXPORT_SYM_class_from_name));
        API_GET_EXPORT_SYM(this->class_get_fields,                OBFUSCATE_STR(SDK_IL2CPP_EXPORT_SYM_class_get_fields));
        API_GET_EXPORT_SYM(this->class_get_methods,               OBFUSCATE_STR(SDK_IL2CPP_EXPORT_SYM_class_get_methods));
        API_GET_EXPORT_SYM(this->domain_assembly_open,            OBFUSCATE_STR(SDK_IL2CPP_EXPORT_SYM_domain_assembly_open));
        API_GET_EXPORT_SYM(this->domain_get,                      OBFUSCATE_STR(SDK_IL2CPP_EXPORT_SYM_domain_get));
        API_GET_EXPORT_SYM(this->image_get_class,                 OBFUSCATE_STR(SDK_IL2CPP_EXPORT_SYM_image_get_class));
        API_GET_EXPORT_SYM(this->object_new,                      OBFUSCATE_STR(SDK_IL2CPP_EXPORT_SYM_object_new));
        API_GET_EXPORT_SYM(this->resolve_icall,                   OBFUSCATE_STR(SDK_IL2CPP_EXPORT_SYM_resolve_icall));
        API_GET_EXPORT_SYM(this->string_new,                      OBFUSCATE_STR(SDK_IL2CPP_EXPORT_SYM_string_new));
        API_GET_EXPORT_SYM(this->thread_attach,                   OBFUSCATE_STR(SDK_IL2CPP_EXPORT_SYM_thread_attach));
        API_GET_EXPORT_SYM(this->type_get_class_or_element_class, OBFUSCATE_STR(SDK_IL2CPP_EXPORT_SYM_type_get_class_or_element_class));

        bool bInitialized = this->array_new                       &&
                            this->class_from_name                 &&
                            this->class_get_fields                &&
                            this->class_get_methods               &&
                            this->domain_assembly_open            &&
                            this->domain_get                      &&
                            this->image_get_class                 &&
                            this->object_new                      &&
                            this->resolve_icall                   &&
                            this->string_new                      &&
                            this->thread_attach                   &&
                            this->type_get_class_or_element_class;

        this->m_bInitialized = bInitialized;
        return bInitialized;
        
        #undef API_GET_EXPORT_PTR
        #undef API_GET_EXPORT_SYM
    }
}
