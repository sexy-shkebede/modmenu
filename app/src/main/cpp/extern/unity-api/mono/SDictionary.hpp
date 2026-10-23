#pragma once

#include "SArray.hpp"

namespace mono
{
    template<typename K, typename V>
    struct SDictionary
    {
        struct SEntry
        {
            int32_t m_iHashCode;
            int32_t m_iNext;
            K m_key;
            V m_value;

            SEntry() = default;
            ~SEntry() = default;
        };

    private:
        char pad[sizeof(void*) * 2];
    public:
        SArray<int32_t>* m_aBuckets;
        SArray<SEntry>* m_aEntries;
        int32_t m_iCount;
        int32_t m_iVersion;
        int32_t m_iFreeList;
        int32_t m_iFreeCount;
        void* m_compare;
        SArray<K>* m_aKeys;
        SArray<V>* m_aValues;
        void* m_syncRoot;

        SEntry& Get(const uint32_t& index)
        {
            return this->m_aEntries->Get(index);
        }

        const SEntry& Get(const uint32_t& index) const
        {
            return this->m_aEntries->Get(index);
        }

        bool IsValid() const
        {
            if (this != nullptr && this->m_iCount > 0) 
            {
                if (this->m_aEntries != nullptr) 
                {
                    return this->m_aEntries->IsValid();
                }
            }
            return false;
        }

        uint32_t GetSize() const
        {
            if (this->IsValid()) 
            {
                return this->m_aEntries->m_iCapacity;
            }
            return 0;
        }

        V& TryGetValue(const K& key)
        {
            for (uint32_t i = 0; i < this->GetSize(); ++i) 
            {
                if (key == this->Get(i).m_key)
                    return this->Get(i).m_value;
            }
            static V empty{};
            return empty;
        }

        SEntry* begin() { return this->IsValid() ? m_aEntries->begin() : nullptr; }
        SEntry* end() { return this->IsValid() ? (m_aEntries->begin() + m_iCount) : nullptr; }
        const SEntry* begin() const { return this->IsValid() ? m_aEntries->begin() : nullptr; }
        const SEntry* end() const { return this->IsValid() ? (m_aEntries->begin() + m_iCount) : nullptr; }

        std::vector<V> ToVector() const
        {
            std::vector<V> ret;
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
