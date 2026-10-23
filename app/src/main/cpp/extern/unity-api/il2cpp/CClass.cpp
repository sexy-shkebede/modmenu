#include "CClass.hpp"
#include "CField.hpp"
#include "CIl2cppApi.hpp"
#include <string.h>
#include "CAssembly.hpp"
#include "../sdk_core.hpp"

namespace il2cpp
{
    CClass* CClass::FromName(const char* pszClassName, const char* pszNamespace, const char* pszAssembly)
    {
        CAssembly* pAssembly = (CAssembly*)il2cpp::Api.domain_assembly_open(nullptr, pszAssembly);
        if(pAssembly != nullptr)
        {
            const CImage* pImage = pAssembly->GetImage();
            CClass* ret = il2cpp::Api.class_from_name(pImage, pszNamespace, pszClassName);
            return ret;
        } 
        return nullptr;
    }

    CMethod* CClass::GetMethods(void** iter)
    {
        return (CMethod*)il2cpp::Api.class_get_methods(this, iter);
    }

    CMethod* CClass::GetMethodFromName(const char* pszName, uint32_t nArgCount, uint8_t nSkip)
    {
        void* iter{};
        uint8_t counter{};
        while(CMethod* pMethod = this->GetMethods(&iter))
        {
            if(pMethod->GetName() && !strcmp(pMethod->GetName(), pszName) && pMethod->GetArgCount() == nArgCount)
            {
                if(counter == nSkip)
                {
                    return pMethod;
                }
                ++counter;
            }
        }
        return nullptr;
    }

    CMethod* CClass::GetMethodContainsName(const char* pszName, uint32_t nArgCount, uint8_t nSkip)
    {
        void* iter{};
        uint8_t counter{};
        while(CMethod* pMethod = this->GetMethods(&iter))
        {
            if(pMethod->GetName() && strstr(pMethod->GetName(), pszName) && pMethod->GetArgCount() == nArgCount)
            {
                if(counter == nSkip)
                {
                    return pMethod;
                }
                ++counter;
            }
        }
        return nullptr;
    }

    CField* CClass::GetFields(void** iter)
    {
        return (CField*)il2cpp::Api.class_get_fields(this, iter);
    }

    CField* CClass::GetFieldFromName(const char* pszName)
    {
        void* iter{};
        while(CField* pField = this->GetFields(&iter))
        {
            if(pField->GetName() && !strcmp(pField->GetName(), pszName))
            {
                return pField;
            }
        }
        return nullptr;
    }
} // il2cpp
