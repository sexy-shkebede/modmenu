#pragma once

#include "CComponent.hpp"

#include "../sdk_fwd.hpp"

namespace usdk
{
    enum ETextAlignmentOptions : int
    {
        eTopLeft = 257,
        eTop = 258,
        eTopRight = 260,
        eTopJustified = 264,
        eTopFlush = 272,
        eTopGeoAligned = 288,
        eLeft = 513,
        eCenter = 514,
        eRight = 516,
        eJustified = 520,
        eFlush = 528,
        eCenterGeoAligned = 544,
        eBottomLeft = 1025,
        eBottom = 1026,
        eBottomRight = 1028,
        eBottomJustified = 1032,
        eBottomFlush = 1040,
        eBottomGeoAligned = 1056,
        eBaselineLeft = 2049,
        eBaseline = 2050,
        eBaselineRight = 2052,
        eBaselineJustified = 2056,
        eBaselineFlush = 2064,
        eBaselineGeoAligned = 2080,
        eMidlineLeft = 4097,
        eMidline = 4098,
        eMidlineRight = 4100,
        eMidlineJustified = 4104,
        eMidlineFlush = 4112,
        eMidlineGeoAligned = 4128,
        eCaplineLeft = 8193,
        eCapline = 8194,
        eCaplineRight = 8196,
        eCaplineJustified = 8200,
        eCaplineFlush = 8208,
        eCaplineGeoAligned = 8224,
        eConverted = 65535
    };

    class CTMP_Text : public CComponent
    {
    public:
        static il2cpp::CClass* GetIl2cppClass();
        static mono::CType*    GetMonoType();

        //mono::SString* GetText();
        void SetText(const char* pszText);
        //void* GetFont();
        void SetFont(void* pFontAsset);
        //float GetFontSize();
        void SetFontSize(float fSize);
        void SetColor(Color color);
        void SetAligment(ETextAlignmentOptions iAlignmentOptions);
        void SetWrapping(bool bState);
    };

    class CTextMeshPro : public CTMP_Text
    {
    };
} // usdk
