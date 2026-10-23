#include "CRoom.hpp"
#include "../../il2cpp/CClass.hpp"
#include "../../il2cpp/CObject.hpp"
#include "../../mono/CType.hpp"
#include "../../mono/SString.hpp"
#include "../../mono/SArray.hpp"
#include "../../sdk_core.hpp"

namespace photonsdk
{
    il2cpp::CClass* CRoom::GetIl2cppClass()
    {
        RESOLVE_SDK_IL2CPP_CLASS("Room", "Photon.Realtime", "PhotonUnityNetworking.dll");
    }

    mono::SString* CRoom::GetName()
    {
        RESOLVE_SDK_METHOD(mono::SString* (*)( CRoom* ), this->GetIl2cppClass(), "get_Name", 0);
        return func(this);
    }
}
