#include "ICall.hpp"
#include "CIl2cppApi.hpp"
#include "../sdk_core.hpp"

namespace il2cpp
{
    uint64_t resolveICall(const char* pszSignature)
    {
        return (uint64_t)il2cpp::Api.resolve_icall(pszSignature);
    }
} // il2cpp
