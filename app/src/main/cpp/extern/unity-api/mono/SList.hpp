#pragma once

#include "SArray.hpp"

namespace mono
{
    template<typename T> struct SArray;
    template <typename T>
    struct SList 
    {
    private:
        char pad[sizeof(void*) * 2];
    public:
        SArray<T>* m_aItems;
        int32_t m_iSize;
        int32_t m_iVersion;

        T& Get(const uint32_t& index) 
        {
            return this->m_aItems->Get(index);
        }

        const T& Get(const uint32_t& index) const
        {
            return this->m_aItems->Get(index);
        }

        bool IsValid() const
        {
            if (this != nullptr && this->m_iSize > 0) 
            {
                if (this->m_aItems != nullptr) 
                {
                    return this->m_aItems->IsValid();
                }
            }
            return false;
        }

        uint32_t GetSize() const
        {
            if (this->IsValid())
            {
                return this->m_aItems->m_iCapacity;
            }
            return 0;
        }

        int32_t GetCount() const
        {
            return m_iSize;
        }

        T* begin() { return this->IsValid() ? m_aItems->begin() : nullptr; }
        T* end() { return this->IsValid() ? (m_aItems->begin() + m_iSize) : nullptr; }
        const T* begin() const { return this->IsValid() ? m_aItems->begin() : nullptr; }
        const T* end() const { return this->IsValid() ? (m_aItems->begin() + m_iSize) : nullptr; }

        std::vector<T> ToVector() const
        {
            std::vector<T> ret;
            if (this->IsValid())
            {
                ret.reserve(this->m_iSize);
                for (int32_t i = 0; i < this->m_iSize; ++i)
                {
                    ret.push_back(this->Get(i));
                }
            }
            return ret;
        }
    };
} // mono
