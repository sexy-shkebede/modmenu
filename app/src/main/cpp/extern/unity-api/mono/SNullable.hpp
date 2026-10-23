#pragma once

namespace mono
{
    template <typename T>
    struct SNullable
    {
        bool m_bHasValue;
        T m_Value;           

        SNullable() : m_bHasValue(false), m_Value() {}
        SNullable(const T& val) : m_bHasValue(true), m_Value(val) {}

        inline explicit operator bool() const { return m_bHasValue; }
        inline T&       operator*()           { return m_Value;     }
        inline const T& operator*()     const { return m_Value;     }
        inline T*       operator->()          { return &m_Value;    }
        inline const T* operator->()    const { return &m_Value;    }

        inline SNullable& operator=(const T& val)
        {
            m_bHasValue = true;
            m_Value = val;
            return *this;
        }
    };
}
