#pragma once

#include "SArray.hpp"

namespace mono
{
    template<typename T>
    struct SHashset {

        struct SSlot 
        {
            int32_t m_iHashcode;
            int32_t m_iNext;
            T m_value;

            SSlot() = default;
            ~SSlot() = default;
        };

    private:
        char pad[sizeof(void*) * 2];
    public:
        SArray<int>* m_aBuckets;
        SArray<SSlot>* m_aSlots;
        int32_t m_iCount;
        int32_t m_iLastIndex;
        int32_t m_iFreeList;
        void* m_comparer;
        int32_t m_iVersion;
        void* m_info;

        SSlot& Get(const uint32_t& index) 
        {
            return this->m_aSlots->Get(index);
        }

        const SSlot& Get(const uint32_t& index) const
        {
            return this->m_aSlots->Get(index);
        }

        bool IsValid() const {
            if (this != nullptr && this->m_iCount > 0) 
            {
                if (this->m_aSlots != nullptr) 
                {
                    return this->m_aSlots->IsValid();
                }
            }
            return false;
        }

        uint32_t GetSize() const
        {
            if (this->IsValid())
            {
                return this->m_aSlots->m_iCapacity;
            }
            return 0;
        }

        SSlot* begin() { return this->IsValid() ? m_aSlots->begin() : nullptr; }
        SSlot* end() { return this->IsValid() ? (m_aSlots->begin() + m_iCount) : nullptr; }
        const SSlot* begin() const { return this->IsValid() ? m_aSlots->begin() : nullptr; }
        const SSlot* end() const { return this->IsValid() ? (m_aSlots->begin() + m_iCount) : nullptr; }

        std::vector<T> ToVector() const
        {
            std::vector<T> ret;
            if (this->IsValid())
            {
                ret.reserve(this->m_iCount);
                for (uint32_t i = 0; i < this->m_iCount; ++i)
                {
                    ret.push_back(this->Get(i).m_value);
                }
            }
            return ret;
        }

    };
} // mono
