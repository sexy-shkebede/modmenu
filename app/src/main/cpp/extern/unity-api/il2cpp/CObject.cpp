#include "CObject.hpp"
#include "CIl2cppApi.hpp"
#include "../sdk_core.hpp"

namespace il2cpp
{
    CObject* CObject::Allocate(const CClass* pClass) 
    {
        return (CObject*)il2cpp::Api.object_new(pClass);
    }
} // il2cpp
