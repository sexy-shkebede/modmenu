#include "brainview.hpp"
#include "sim.hpp"

#include "imgui_internal.h"   // ImGui::RegisterUserTexture, ImTextureDataQueueUpload

#include <math.h>
#include <string>
#include <utility>
#include <vector>

namespace
{
    // Окно проекции, мкм: мозг FlyWire ~ 90..900 по x, 50..445 по y
    constexpr float kX0 = 80.f, kY0 = 40.f, kUmPerPx = 2.17f;
    constexpr float kTau = 0.15f;              // с: как быстро гаснет вспышка нейрона
    constexpr float kUpload = 1.f / 15.f;      // с между обновлениями текстуры

    struct SView
    {
        ImTextureData*        pTex = nullptr;
        std::vector<int32_t>  pixel;           // нейрон → пиксель (-1 — нет координат)
        std::vector<float>    count;           // нейронов в пикселе (размыто)
        std::vector<float>    acc, blur;       // недавние спайки в пикселе
        std::vector<uint32_t> spikes;
        std::vector<std::pair<std::string, ImVec2>> groups;
        float                 fSinceUpload = 1.f;
        unsigned char         aLinear[256];
    };
    SView V;

    // Размытие [1 2 1] / 4 по обеим осям (вспышка одного нейрона — пятно, а не пиксель)
    void Blur(const std::vector<float>& in, std::vector<float>& out)
    {
        const int W = fly::kViewW, H = fly::kViewH;
        static std::vector<float> tmp;
        tmp.resize(in.size());
        out.resize(in.size());
        for (int y = 0; y < H; y++)
            for (int x = 0; x < W; x++)
            {
                const float* r = &in[(size_t)y * W];
                tmp[(size_t)y * W + x] = 0.5f * r[x] + 0.25f * (r[x > 0 ? x - 1 : x] + r[x < W - 1 ? x + 1 : x]);
            }
        for (int y = 0; y < H; y++)
            for (int x = 0; x < W; x++)
            {
                const size_t i = (size_t)y * W + x;
                out[i] = 0.5f * tmp[i] + 0.25f * (tmp[y > 0 ? i - W : i] + tmp[y < H - 1 ? i + W : i]);
            }
    }

    struct SRgb { float r, g, b; };
    SRgb Hex(unsigned c) { return { ((c >> 16) & 0xFF) / 255.f, ((c >> 8) & 0xFF) / 255.f, (c & 0xFF) / 255.f }; }
    SRgb Mix(SRgb a, SRgb b, float t) { return { a.r + (b.r - a.r) * t, a.g + (b.g - a.g) * t, a.b + (b.b - a.b) * t }; }

    // Тепловая шкала (sRGB): красно-оранжевый → янтарь → почти белый
    SRgb Heat(float t)
    {
        static const SRgb k0 = Hex(0xE2461C), k1 = Hex(0xFFB22E), k2 = Hex(0xFFF6D6);
        return t < 0.5f ? Mix(k0, k1, t * 2.f) : Mix(k1, k2, (t - 0.5f) * 2.f);
    }

    void Compose()
    {
        const int   n   = fly::kViewW * fly::kViewH;
        const SRgb  den = Hex(0x8FA3C4);
        Blur(V.acc, V.blur);
        unsigned char* d = (unsigned char*)V.pTex->GetPixels();
        for (int i = 0; i < n; i++, d += 4)
        {
            const float c  = V.count[i];
            // фон: плотность нейронов, логарифмически
            const float aD = c > 0.02f ? 0.10f + 0.30f * fminf(logf(1.f + c) / logf(30.f), 1.f) : 0.f;
            // активность: недавние спайки относительно числа нейронов в пикселе
            const float t  = 1.f - expf(-V.blur[i] / (0.35f + 0.06f * c));
            const float aA = fminf(1.f, powf(t, 0.7f) * 1.15f);
            const SRgb  h  = Heat(t);
            const float A  = aA + aD * (1.f - aA);
            if (A <= 0.002f) { d[0] = d[1] = d[2] = d[3] = 0; continue; }
            const float wA = aA / A, wD = aD * (1.f - aA) / A;
            const auto Ch = [&](float a, float b) { const float v = a * wA + b * wD; return V.aLinear[(int)(fminf(fmaxf(v, 0.f), 1.f) * 255.f + 0.5f)]; };
            d[0] = Ch(h.r, den.r);
            d[1] = Ch(h.g, den.g);
            d[2] = Ch(h.b, den.b);
            d[3] = (unsigned char)(A * 255.f + 0.5f);
        }
    }

    bool Project(const int16_t* p, float& u, float& v)
    {
        if (p[0] == 0 && p[1] == 0 && p[2] == 0) return false;   // у нейрона нет координат
        u = ((float)p[0] * 0.25f - kX0) / kUmPerPx;
        v = ((float)p[1] * 0.25f - kY0) / kUmPerPx;
        return u >= 0.f && v >= 0.f && u < (float)fly::kViewW && v < (float)fly::kViewH;
    }
}

ImTextureData* fly::BrainView()
{
    if (V.pTex) return V.pTex;
    const SConnectome* pC = SimConnectome();
    if (!pC) return nullptr;

    // Текстура меню — sRGB, а ImGui пишет цвет как есть: в пикселях — линейные значения
    for (int i = 0; i < 256; i++)
    {
        const float c = (float)i / 255.f;
        V.aLinear[i] = (unsigned char)((c <= 0.04045f ? c / 12.92f : powf((c + 0.055f) / 1.055f, 2.4f)) * 255.f + 0.5f);
    }
    const int n = kViewW * kViewH;
    V.pixel.assign(pC->N, -1);
    std::vector<float> cnt(n, 0.f);
    for (uint32_t j = 0; j < pC->N; j++)
    {
        float u, v;
        if (!Project(pC->pPos + (size_t)j * 3, u, v)) continue;
        V.pixel[j] = (int32_t)v * kViewW + (int32_t)u;
        cnt[V.pixel[j]] += 1.f;
    }
    Blur(cnt, V.count);
    V.acc.assign(n, 0.f);
    V.blur.assign(n, 0.f);

    V.pTex = IM_NEW(ImTextureData)();
    V.pTex->Create(ImTextureFormat_RGBA32, kViewW, kViewH);
    V.pTex->UseColors = true;
    Compose();
    ImGui::RegisterUserTexture(V.pTex);
    return V.pTex;
}

void fly::BrainViewUpdate(float fDt)
{
    if (!BrainView()) return;
    SimTakeSpikes(V.spikes);
    const float k = expf(-fDt / kTau);
    for (float& a : V.acc) a *= k;
    for (uint32_t j : V.spikes)
        if (j < V.pixel.size() && V.pixel[j] >= 0) V.acc[V.pixel[j]] += 1.f;

    V.fSinceUpload += fDt;
    if (V.fSinceUpload < kUpload) return;
    V.fSinceUpload = 0.f;
    Compose();
    ImTextureDataQueueUpload(V.pTex, 0, 0, kViewW, kViewH);
}

bool fly::BrainViewGroup(const char* pszGroup, ImVec2& outUv)
{
    for (const auto& g : V.groups)
        if (g.first == pszGroup) { outUv = g.second; return g.second.x >= 0.f; }

    const SConnectome* pC = SimConnectome();
    if (!pC) return false;
    ImVec2 uv(-1.f, -1.f);
    if (const std::vector<uint32_t>* pIdx = pC->Group(pszGroup))
    {
        double su = 0.0, sv = 0.0;
        int    n  = 0;
        for (uint32_t j : *pIdx)
        {
            float u, v;
            if (!Project(pC->pPos + (size_t)j * 3, u, v)) continue;
            su += u; sv += v; n++;
        }
        if (n) uv = ImVec2((float)(su / n) / (float)kViewW, (float)(sv / n) / (float)kViewH);
    }
    V.groups.emplace_back(pszGroup, uv);
    outUv = uv;
    return uv.x >= 0.f;
}
