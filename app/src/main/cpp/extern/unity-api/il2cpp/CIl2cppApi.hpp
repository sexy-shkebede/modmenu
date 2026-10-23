#pragma once

#include "il2cpp/CClass.hpp"
#include "il2cpp/CField.hpp"
#include "il2cpp/CObject.hpp"
#include "il2cpp/CImage.hpp"
#include "il2cpp/CAssembly.hpp"
#include "il2cpp/CMethod.hpp"
#include "il2cpp/CType.hpp"

namespace mono
{ 
    struct SString;
}

namespace il2cpp
{
    struct CIl2cppApi
    {
    public:
        bool Initialize(void* pIl2cppHandle);
        bool m_bInitialized = false;

        void*                     (*array_new)                       (const il2cpp::CClass*, uintptr_t);
        il2cpp::CClass*           (*class_from_name)                 (const il2cpp::CImage*, const char*, const char*);
        il2cpp::CField*           (*class_get_fields)                (il2cpp::CClass* klass, void* *iter);
        const il2cpp::CMethod*    (*class_get_methods)               (il2cpp::CClass* klass, void* *iter);
        const il2cpp::CAssembly*  (*domain_assembly_open)            (void*, const char*);
        void*                     (*domain_get)                      ();
        const il2cpp::CClass*     (*image_get_class)                 (const il2cpp::CImage* image, size_t index);
        il2cpp::CObject*          (*object_new)                      (const il2cpp::CClass*);
        void*                     (*resolve_icall)                   (const char*);
        mono::SString*            (*string_new)                      (const char*);
        void                      (*thread_attach)                   (void* domain);
        il2cpp::CClass*           (*type_get_class_or_element_class) (const il2cpp::CType*);

        /*
        int                       (*class_get_flags)                 (const il2cpp::CClass* klass);
        il2cpp::CClass*           (*class_get_interfaces)            (const il2cpp::CClass* klass, void** iter);
        const void*               (*class_get_properties)            (const il2cpp::CClass* klass, void** iter);
        bool                      (*class_is_enum)                   (const il2cpp::CClass* klass);
        bool                      (*class_is_valuetype)              (const il2cpp::CClass* klass);
        il2cpp::CAssembly**       (*domain_get_assemblies)           (const void* domain, size_t* size);
        int                       (*field_get_flags)                 (const il2cpp::CField* field);
        void                      (*field_static_get_value)          (const il2cpp::CField* field, void* value);
        const char*               (*method_get_param_name)           (const il2cpp::CMethod* method, uint32_t index);
        il2cpp::CMethod*          (*property_get_get_method)         (const void* prop);
        const char*               (*property_get_name)               (const void* prop);
        il2cpp::CMethod*          (*property_get_set_method)         (const void* prop);
        bool                      (*type_is_byref)                   (const il2cpp::CType* type);
        */
    };
    
    extern CIl2cppApi Api;
} // il2cpp
