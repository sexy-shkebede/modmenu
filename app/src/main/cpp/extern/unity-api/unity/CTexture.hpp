#pragma once

#include "CObject.hpp"
#include "common/include.hpp"

#include "../sdk_fwd.hpp"

namespace usdk
{
    enum ETextureFormat : int 
    {
        eAlpha8 = 1,
        eArgb4444 = 2,
        eRgb24 = 3,
        eRgba32 = 4,
        eArgb32 = 5,
        eRgb565 = 7,
        eR16 = 9,
        eDxt1 = 10,
        eDxt5 = 12,
        eRgba4444 = 13,
        eBgra32 = 14,
        eRhalf = 15,
        eRghalf = 16,
        eRgbahalf = 17,
        eRfloat = 18,
        eRgfloat = 19,
        eRgbafloat = 20,
        eYuy2 = 21,
        eRgb9e5float = 22,
        eBc4 = 26,
        eBc5 = 27,
        eBc6h = 24,
        eBc7 = 25,
        eDxt1crunched = 28,
        eDxt5crunched = 29,
        ePvrtc_rgb2 = 30,
        ePvrtc_rgba2 = 31,
        ePvrtc_rgb4 = 32,
        ePvrtc_rgba4 = 33,
        eEtc_rgb4 = 34,
        eEac_r = 41,
        eEac_r_signed = 42,
        eEac_rg = 43,
        eEac_rg_signed = 44,
        eEtc2_rgb = 45,
        eEtc2_rgba1 = 46,
        eEtc2_rgba8 = 47,
        eAstc_4x4 = 48,
        eAstc_5x5 = 49,
        eAstc_6x6 = 50,
        eAstc_8x8 = 51,
        eAstc_10x10 = 52,
        eAstc_12x12 = 53,
        eEtc_rgb4_3ds = 60,
        eEtc_rgba8_3ds = 61,
        eRg16 = 62,
        eR8 = 63,
        eEtc_rgb4crunched = 64,
        eEtc2_rgba8crunched = 65,
        eAstc_hdr_4x4 = 66,
        eAstc_hdr_5x5 = 67,
        eAstc_hdr_6x6 = 68,
        eAstc_hdr_8x8 = 69,
        eAstc_hdr_10x10 = 70,
        eAstc_hdr_12x12 = 71,
        eAstc_rgb_4x4 = 48,
        eAstc_rgb_5x5 = 49,
        eAstc_rgb_6x6 = 50,
        eAstc_rgb_8x8 = 51,
        eAstc_rgb_10x10 = 52,
        eAstc_rgb_12x12 = 53,
        eAstc_rgba_4x4 = 54,
        eAstc_rgba_5x5 = 55,
        eAstc_rgba_6x6 = 56,
        eAstc_rgba_8x8 = 57,
        eAstc_rgba_10x10 = 58,
        eAstc_rgba_12x12 = 59,
    };

    class CTexture : public CObject 
    {

    };

    class CTexture3D : public CTexture 
    {

    };

    class CTexture2D : public CTexture 
    {
    public:
        static il2cpp::CClass* GetIl2cppClass();
        static mono::CType*    GetMonoType();

        static CTexture2D* GetWhiteTexture();
        static void ctor(CTexture2D* pThiz, int32_t iWidth, int32_t iHeight, ETextureFormat eFormat, bool bMipChain = true);

        mono::SArray<Color>* GetPixels();
        void SetPixels(mono::SArray<Color>* pixels);
        void SetPixel(uint32_t x, uint32_t y, Color color);
        void Apply();
    };
} // usdk
