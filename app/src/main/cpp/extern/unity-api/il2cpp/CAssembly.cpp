#include "CAssembly.hpp"
#include "CIl2cppApi.hpp"
#include "../sdk_core.hpp"

namespace il2cpp
{
    CAssembly* CAssembly::FromName(const char* pszName)
    {
        static CAssembly*(*fn)(const char*) = (decltype(fn))(nullptr/*__engine_il2cpp_get_assembly*/);
        return fn(pszName);
    }
} // il2cpp
