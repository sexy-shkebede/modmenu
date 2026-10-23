#include "CType.hpp"
#include "CIl2cppApi.hpp"
#include "../sdk_core.hpp"

namespace il2cpp
{  
    CClass* CType::GetClassOrElementClass() const
    {
        return (CClass*)il2cpp::Api.type_get_class_or_element_class(this);
    }
} // il2cpp
