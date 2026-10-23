#include "Ray.hpp"
#include "../CObject.hpp"
#include "../../sdk_core.hpp"

namespace usdk
{
    CCollider* SRaycastHit::GetCollider()
    {
        return (CCollider*)CObject::FindObjectFromID(this->m_uColliderInstanceID);
    }
}
