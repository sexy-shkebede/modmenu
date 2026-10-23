#pragma once

#include "Vector3.hpp"

namespace usdk
{
    struct SBounds 
    {
    public:
        Vector3 m_vecCenter;
        Vector3 m_vecExtents;

        Vector3 GetExtents()
        {
            return this->m_vecExtents;
        }

        Vector3 GetCenter()
        {
            return this->m_vecCenter;
        }

        Vector3 GetMin() 
        {
            return this->m_vecCenter - this->m_vecExtents;
        }

        Vector3 GetMax() 
        {
            return this->m_vecCenter + this->m_vecExtents;
        }

        Vector3 GetSize() 
        {
            return this->m_vecExtents * 2;
        }
    };
}
