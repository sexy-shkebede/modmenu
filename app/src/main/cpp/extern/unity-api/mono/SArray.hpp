#pragma once

#include <vector>

namespace il2cpp
{ 
    class CClass; 
    struct CIl2cppApi; 
    extern CIl2cppApi Api; 
}

namespace mono
{
    template<typename T>
    struct SArray 
    {
    private:
        char pad[sizeof(void*) * 2];
    public:
        void* m_pBounds;
        int32_t m_iCapacity;
        T m_aItems[0];

        static SArray<T>* Create(il2cpp::CClass* pClass, size_t uSize);

        T& Get(uint32_t index) 
        {
            return this->m_aItems[index];
        }

        const T& Get(uint32_t index) const 
        {
            return this->m_aItems[index];
        }
   
        uint32_t GetSize() const
        {
            return m_iCapacity;
        }

        bool IsValid() const
        {
            return this != nullptr && m_iCapacity >= 0; 
        }

        T* begin() { return &m_aItems[0]; }
        T* end() { return &m_aItems[m_iCapacity]; }
        const T* begin() const { return &m_aItems[0]; }
        const T* end() const { return &m_aItems[m_iCapacity]; }

        std::vector<T> ToVector() const
        {
            std::vector<T> ret;
            if (this->IsValid()) 
            {
                ret.reserve(this->m_iCapacity);
                for (uint32_t i = 0; i < this->m_iCapacity; ++i) 
                {
                    ret.Add(this->Get(i));
                }
            }
            return ret;
        }
    };
} // mono

#include "../il2cpp/CIl2cppApi.hpp" 

namespace mono
{
    template<typename T>
    SArray<T>* SArray<T>::Create(il2cpp::CClass* pClass, size_t uSize)
    {
        return (SArray<T>*)il2cpp::Api.array_new(pClass, uSize);
    }
}
