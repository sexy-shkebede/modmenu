#pragma once

namespace mono
{
    struct SDelegate
    {
    private:
        char pad[sizeof(void*)*2];
    public:
        uint64_t m_pMethod;
        uint64_t m_pInvokeImpl;
        uint64_t m_pTarget; 
        uint64_t m_method;
        uint64_t m_pDelegateTrampoline;
        uint64_t m_extraArg;
        uint64_t m_pMethodCode; 
        uint64_t m_pInterpMethod; 
        uint64_t m_pInterpInvokeImpl;
        uint64_t m_pMethodInfo;
        uint64_t m_pOriginalMethodInfo;
        uint64_t m_pData;
        bool m_bVirtual;

        inline void Hook(uint64_t hk, uint64_t* origin)
        {
            if(origin != nullptr) {
                if(*origin == 0)
                    *origin = m_pInvokeImpl;
            }
            m_pInvokeImpl = hk;
        }
    };
} // mono
