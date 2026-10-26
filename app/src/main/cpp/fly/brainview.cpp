#include "brainview.hpp"
#include "sim.hpp"

#include "imgui_internal.h"   // ImGui::RegisterUserTexture, ImTextureDataQueueUpload

#include <math.h>
#include <string>
#include <utility>
#include <vector>

namespace
{
    // Окно проекции мозга, мкм: мозг FlyWire ~ 90..900 по x, 50..445 по y
    constexpr float kX0 = 80.f, kY0 = 40.f, kUmPerPx = 2.17f;
    // Окно цепочки BANC, мкм: голова → брюшко по y 525..1075, поперёк по x 320..640
    constexpr float kCordY0 = 525.f, kCordX0 = 320.f, kCordUmPerPx = 2.15f;
    constexpr float kTau = 0.15f;              // с: как быстро гаснет вспышка нейрона
    constexpr float kUpload = 1.f / 15.f;      // с между обновлениями текстуры

    struct SView
    {
        int                   W = 0, H = 0;
        ImTextureData*        pTex = nullptr;
        std::vector<int32_t>  pixel;           // нейрон → пиксель (-1 — нет координат)
        std::vector<float>    count;           // нейронов в пикселе (размыто)
        std::vector<float>    acc, blur;       // недавняя активность в пикселе (спайки за ~kTau)
        std::vector<std::pair<std::string, ImVec2>> groups;
        float                 fSinceUpload = 1.f;
    };
    SView                 B, C;                // мозг и цепочка
    std::vector<uint32_t> g_Spikes;
    std::vector<float>    g_Rates;
    bool                  g_bCordFlip = false; // левый бок мухи — снизу картинки
    unsigned char         g_aLinear[256];

    // Размытие [1 2 1] / 4 по обеим осям (вспышка одного нейрона — пятно, а не пиксель)
    void Blur(const SView& v, const std::vector<float>& in, std::vector<float>& out)
    {
        const int W = v.W, H = v.H;
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

    void Compose(SView& v)
    {
        const int   n   = v.W * v.H;
        const SRgb  den = Hex(0x8FA3C4);
        Blur(v, v.acc, v.blur);
        unsigned char* d = (unsigned char*)v.pTex->GetPixels();
        for (int i = 0; i < n; i++, d += 4)
        {
            const float c  = v.count[i];
            // фон: плотность нейронов, логарифмически
            const float aD = c > 0.02f ? 0.10f + 0.30f * fminf(logf(1.f + c) / logf(30.f), 1.f) : 0.f;
            // активность: недавние спайки относительно числа нейронов в пикселе
            const float t  = 1.f - expf(-v.blur[i] / (0.35f + 0.06f * c));
            const float aA = fminf(1.f, powf(t, 0.7f) * 1.15f);
            const SRgb  h  = Heat(t);
            const float A  = aA + aD * (1.f - aA);
            if (A <= 0.002f) { d[0] = d[1] = d[2] = d[3] = 0; continue; }
            const float wA = aA / A, wD = aD * (1.f - aA) / A;
            const auto Ch = [&](float a, float b) { const float x = a * wA + b * wD; return g_aLinear[(int)(fminf(fmaxf(x, 0.f), 1.f) * 255.f + 0.5f)]; };
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

    // Цепочка сверху: вдоль картинки — от головы к брюшку, поперёк — бок мухи
    bool ProjectCord(const int16_t* p, float& u, float& v)
    {
        if (p[0] == 0 && p[1] == 0 && p[2] == 0) return false;
        u = ((float)p[1] * 0.25f - kCordY0) / kCordUmPerPx;
        v = ((float)p[0] * 0.25f - kCordX0) / kCordUmPerPx;
        if (g_bCordFlip) v = (float)fly::kCordH - 1e-3f - v;
        return u >= 0.f && v >= 0.f && u < (float)fly::kCordW && v < (float)fly::kCordH;
    }

    void InitLinear()
    {
        // Текстура меню — sRGB, а ImGui пишет цвет как есть: в пикселях — линейные значения
        for (int i = 0; i < 256; i++)
        {
            const float c = (float)i / 255.f;
            g_aLinear[i] = (unsigned char)((c <= 0.04045f ? c / 12.92f : powf((c + 0.055f) / 1.055f, 2.4f)) * 255.f + 0.5f);
        }
    }

    template <class FProject>
    void Build(SView& v, int W, int H, const int16_t* pPos, uint32_t n, FProject project)
    {
        InitLinear();
        v.W = W;
        v.H = H;
        v.pixel.assign(n, -1);
        std::vector<float> cnt((size_t)W * H, 0.f);
        for (uint32_t j = 0; j < n; j++)
        {
            float u, w;
            if (!project(pPos + (size_t)j * 3, u, w)) continue;
            v.pixel[j] = (int32_t)w * W + (int32_t)u;
            cnt[v.pixel[j]] += 1.f;
        }
        Blur(v, cnt, v.count);
        v.acc.assign(cnt.size(), 0.f);
        v.blur.assign(cnt.size(), 0.f);
        v.pTex = IM_NEW(ImTextureData)();
        v.pTex->Create(ImTextureFormat_RGBA32, W, H);
        v.pTex->UseColors = true;
        Compose(v);
        ImGui::RegisterUserTexture(v.pTex);
    }

    void Upload(SView& v, float fDt)
    {
        v.fSinceUpload += fDt;
        if (v.fSinceUpload < kUpload) return;
        v.fSinceUpload = 0.f;
        Compose(v);
        ImTextureDataQueueUpload(v.pTex, 0, 0, v.W, v.H);
    }

    template <class FProject>
    bool Group(SView& v, const std::vector<uint32_t>* pIdx, const int16_t* pPos, uint32_t n, const char* pszGroup, ImVec2& outUv, FProject project)
    {
        ImVec2 uv(-1.f, -1.f);
        if (pIdx)
        {
            double su = 0.0, sv = 0.0;
            int    k  = 0;
            for (uint32_t j : *pIdx)
            {
                float u, w;
                if (j >= n || !project(pPos + (size_t)j * 3, u, w)) continue;
                su += u; sv += w; k++;
            }
            if (k) uv = ImVec2((float)(su / k) / (float)v.W, (float)(sv / k) / (float)v.H);
        }
        v.groups.emplace_back(pszGroup, uv);
        outUv = uv;
        return uv.x >= 0.f;
    }
}

ImTextureData* fly::BrainView()
{
    if (B.pTex) return B.pTex;
    const SConnectome* pC = SimConnectome();
    if (!pC) return nullptr;
    Build(B, kViewW, kViewH, pC->pPos, pC->N, Project);
    return B.pTex;
}

void fly::BrainViewUpdate(float fDt)
{
    if (!BrainView()) return;
    SimTakeSpikes(g_Spikes);
    const float k = expf(-fDt / kTau);
    for (float& a : B.acc) a *= k;
    for (uint32_t j : g_Spikes)
        if (j < B.pixel.size() && B.pixel[j] >= 0) B.acc[B.pixel[j]] += 1.f;
    Upload(B, fDt);
}

bool fly::BrainViewGroup(const char* pszGroup, ImVec2& outUv)
{
    for (const auto& g : B.groups)
        if (g.first == pszGroup) { outUv = g.second; return g.second.x >= 0.f; }
    const SConnectome* pC = SimConnectome();
    if (!pC) return false;
    return Group(B, pC->Group(pszGroup), pC->pPos, pC->N, pszGroup, outUv, Project);
}

ImTextureData* fly::CordView()
{
    if (C.pTex) return C.pTex;
    const SVncData* pD = SimVncData();
    if (!pD) return nullptr;
    // какой стороной x в BANC лежит левый бок мухи: по мотонейронам передних ног
    const auto MeanX = [&](const char* psz)
    {
        const std::vector<uint32_t>* g = pD->Group(psz);
        double s = 0.0;
        int    n = 0;
        if (g) for (uint32_t j : *g) if (j < pD->Nv) { s += pD->pPos[(size_t)j * 3]; n++; }
        return n ? s / n : 0.0;
    };
    g_bCordFlip = MeanX("L1_all") < MeanX("R1_all");
    Build(C, kCordW, kCordH, pD->pPos, pD->Nv, ProjectCord);
    return C.pTex;
}

void fly::CordViewUpdate(float fDt)
{
    if (!CordView()) return;
    // частоты → "спайков за kTau", как у мозга; новые — раз в ~1/15 с (их отдаёт симуляция)
    if (C.fSinceUpload + fDt >= kUpload)
    {
        SimVncRates(g_Rates);
        for (float& a : C.acc) a = 0.f;
        for (size_t j = 0; j < g_Rates.size() && j < C.pixel.size(); j++)
            if (C.pixel[j] >= 0 && g_Rates[j] > 0.f) C.acc[C.pixel[j]] += g_Rates[j] * kTau;
    }
    Upload(C, fDt);
}

bool fly::CordViewGroup(const char* pszGroup, ImVec2& outUv)
{
    for (const auto& g : C.groups)
        if (g.first == pszGroup) { outUv = g.second; return g.second.x >= 0.f; }
    const SVncData* pD = SimVncData();
    if (!pD || !C.pTex) return false;
    return Group(C, pD->Group(pszGroup), pD->pPos, pD->Nv, pszGroup, outUv, ProjectCord);
}
