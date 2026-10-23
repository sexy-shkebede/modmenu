#pragma once

namespace il2cpp
{
    class CClass;

    class CObject 
    {
    public:
        CClass *m_pKlass;
        CClass *m_pVTable;

        static CObject* Allocate(const CClass* pClass);
    };
} // il2cpp
