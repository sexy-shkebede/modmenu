#include "CImage.hpp"
#include "CIl2cppApi.hpp"
#include "../sdk_core.hpp"

namespace il2cpp
{

CClass* CImage::GetClass(uint32_t nIndex)
{
    return (CClass*)il2cpp::Api.image_get_class(this, nIndex);
}

} // il2cpp
