#include "draw.hpp"
#include "../openxr_hook/world_view.hpp"

#include "imgui_internal.h"

#include <algorithm>
#include <math.h>
#include <unistd.h>
#include <vector>

namespace fly
{
namespace
{
    struct V3 { float x, y, z; };
    V3    operator+(V3 a, V3 b)    { return { a.x + b.x, a.y + b.y, a.z + b.z }; }
    V3    operator-(V3 a, V3 b)    { return { a.x - b.x, a.y - b.y, a.z - b.z }; }
    V3    operator*(V3 a, float s) { return { a.x * s, a.y * s, a.z * s }; }
    V3    operator-(V3 a)          { return { -a.x, -a.y, -a.z }; }
    float Dot(V3 a, V3 b)          { return a.x * b.x + a.y * b.y + a.z * b.z; }
    V3    Cross(V3 a, V3 b)        { return { a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x }; }
    V3    Norm(V3 a)               { const float l = sqrtf(Dot(a, a)); return l > 1e-12f ? a * (1.f / l) : V3{ 0.f, 1.f, 0.f }; }
    V3    P3(const float* p)       { return { p[0], p[1], p[2] }; }

    V3 Rot(const float q[4], V3 v)
    {
        const float x2 = q[0] * 2.f, y2 = q[1] * 2.f, z2 = q[2] * 2.f;
        const float xx = q[0] * x2, yy = q[1] * y2, zz = q[2] * z2;
        const float xy = q[0] * y2, xz = q[0] * z2, yz = q[1] * z2;
        const float wx = q[3] * x2, wy = q[3] * y2, wz = q[3] * z2;
        return { v.x * (1.f - (yy + zz)) + v.y * (xy - wz)         + v.z * (xz + wy),
                 v.x * (xy + wz)         + v.y * (1.f - (xx + zz)) + v.z * (yz - wx),
                 v.x * (xz - wy)         + v.y * (yz + wx)         + v.z * (1.f - (xx + yy)) };
    }

    // Аффинное преобразование: p' = ax·p.x + ay·p.y + az·p.z + t
    struct SAffine
    {
        V3 ax, ay, az, t;
        V3 Dir  (V3 v) const { return ax * v.x + ay * v.y + az * v.z; }
        V3 Point(V3 p) const { return Dir(p) + t; }
    };
    SAffine Then(const SAffine& first, const SAffine& second)   // second(first(p))
    {
        return { second.Dir(first.ax), second.Dir(first.ay), second.Dir(first.az), second.Point(first.t) };
    }

    // Мир Unity → пространство трекинга игры (OpenXR): обратный риг и смена руки (Z → −Z)
    SAffine WorldToLayer(const SRig& rig)
    {
        const float qInv[4] = { -rig.aRot[0], -rig.aRot[1], -rig.aRot[2], rig.aRot[3] };
        const float s       = rig.fScale > 1e-4f ? 1.f / rig.fScale : 1.f;
        const auto  Axis    = [&](V3 e) { const V3 r = Rot(qInv, e) * s; return V3{ r.x, r.y, -r.z }; };
        SAffine a = { Axis({ 1.f, 0.f, 0.f }), Axis({ 0.f, 1.f, 0.f }), Axis({ 0.f, 0.f, 1.f }), { 0.f, 0.f, 0.f } };
        a.t = -a.Dir({ rig.aPos[0], rig.aPos[1], rig.aPos[2] });
        return a;
    }

    // Пространство слоя → оси глаза (OpenXR: −Z вперёд)
    SAffine LayerToEye(const SWorldEye& e)
    {
        const float qInv[4] = { -e.aRot[0], -e.aRot[1], -e.aRot[2], e.aRot[3] };
        SAffine a = { Rot(qInv, { 1.f, 0.f, 0.f }), Rot(qInv, { 0.f, 1.f, 0.f }), Rot(qInv, { 0.f, 0.f, 1.f }), { 0.f, 0.f, 0.f } };
        a.t = -a.Dir({ e.aPos[0], e.aPos[1], e.aPos[2] });
        return a;
    }

    float Clamp01(float t) { return t < 0.f ? 0.f : (t > 1.f ? 1.f : t); }

    // ---------------------------------------------------------------- цвет
    float Lin(float c) { return c <= 0.04045f ? c / 12.92f : powf((c + 0.055f) / 1.055f, 2.4f); }

    struct SCol { float r, g, b, a; };
    SCol Rgb(unsigned rgb, float a = 1.f) { return { ((rgb >> 16) & 0xFF) / 255.f, ((rgb >> 8) & 0xFF) / 255.f, (rgb & 0xFF) / 255.f, a }; }

    ImU32 Shade(SCol c, float k)
    {
        // Текстура слоя — sRGB, ImGui пишет как есть: цвет — линейный (как ui::Col)
        const auto Ch = [&](float x) { const float v = Lin(x) * k; return (int)(fminf(fmaxf(v, 0.f), 1.f) * 255.f + 0.5f); };
        return IM_COL32(Ch(c.r), Ch(c.g), Ch(c.b), (int)(fminf(fmaxf(c.a, 0.f), 1.f) * 255.f + 0.5f));
    }

    // ---------------------------------------------------------------- треугольники
    struct STri
    {
        V3    p[3];        // мир, потом — оси глаза
        V3    n;           // наружу (мир)
        SCol  col;
        float fSpec;       // блик (глаза)
        bool  bTwoSided;   // крылья
        float fDepth;
    };
    std::vector<STri> g_World;   // треугольники кадра в мире (рендер-поток)
    std::vector<STri> g_Eye;

    void Tri(V3 a, V3 b, V3 c, V3 center, SCol col, float fSpec = 0.f, bool bTwo = false)
    {
        V3 n = Norm(Cross(b - a, c - a));
        if (Dot(n, (a + b + c) * (1.f / 3.f) - center) < 0.f) n = -n;
        g_World.push_back({ { a, b, c }, n, col, fSpec, bTwo, 0.f });
    }

    struct SXf   // оси мухи (мм) → мир
    {
        V3 o, ax, ay, az;
        V3 operator()(V3 p) const { return o + ax * p.x + ay * p.y + az * p.z; }
    };

    SCol PartColor(uint8_t part)
    {
        switch (part)
        {
            case kHead:      return Rgb(0x8E6E45);
            case kEye:       return Rgb(0xA01818);
            case kThorax:    return Rgb(0x87663F);
            case kScutellum: return Rgb(0x6E5233);
            case kAbdomen:   return Rgb(0xB08D58);
            case kLeg:       return Rgb(0x4E3B28);
            case kAntenna:   return Rgb(0x7A5A36);
            case kProboscis: return Rgb(0x8A6C4C);
            case kLabellum:  return Rgb(0xA88A66);
            case kHaltere:   return Rgb(0xD2BF94);
            case kDrop:      return Rgb(0xFFC24A, 0.62f);
            default:         return Rgb(0x808080);
        }
    }

    void Ellipsoid(const SXf& X, const SEllipsoid& e)
    {
        const int nLat = e.uPart == kEye ? 9 : 10, nLon = e.uPart == kEye ? 12 : 16;
        const V3  c = P3(e.c);
        const V3  center = X(c);
        const auto Vert = [&](int i, int j, V3& out, V3& unit)
        {
            const float th = 3.14159265f * (float)i / (float)nLat, ph = 2.f * 3.14159265f * (float)j / (float)nLon;
            unit = { sinf(th) * cosf(ph), cosf(th), sinf(th) * sinf(ph) };
            out  = X(c + Rot(e.q, { unit.x * e.r[0], unit.y * e.r[1], unit.z * e.r[2] }));
        };
        const SCol base = PartColor(e.uPart);
        for (int i = 0; i < nLat; i++)
            for (int j = 0; j < nLon; j++)
            {
                V3 a, b, cc, d, ua, ub, uc, ud;
                Vert(i, j, a, ua); Vert(i, j + 1, b, ub); Vert(i + 1, j + 1, cc, uc); Vert(i + 1, j, d, ud);
                const V3 um = (ua + ub + uc + ud) * 0.25f;   // точка ячейки на единичной сфере (до поворота)
                SCol col = base;
                float spec = 0.f;
                if (e.uPart == kEye)
                {
                    // фасетки: шахматка светлее/темнее
                    const float k = ((i + j) & 1) ? 1.f : 0.78f;
                    col = { col.r * k, col.g * k, col.b * k, 1.f };
                    spec = 0.35f;
                }
                else if (e.uPart == kAbdomen)
                {
                    // тергиты: тёмные полосы по заднему краю сегментов сверху, светлое брюшко снизу
                    const float band = (um.z + 1.f) * 3.f;
                    if (um.y > -0.3f && band - floorf(band) < 0.45f && um.z < 0.75f) col = Rgb(0x4A3420);
                    if (um.y < -0.55f) col = Rgb(0xC9B089);
                }
                if (i > 0)          Tri(a, b, cc, center, col, spec);
                if (i < nLat - 1)   Tri(a, cc, d, center, col, spec);
            }
    }

    void Segment(const SXf& X, const SSegment& s)
    {
        const int n = 6;
        const V3  a = X(P3(s.a)), b = X(P3(s.b));
        V3 axis = b - a;
        const float len = sqrtf(Dot(axis, axis));
        if (len < 1e-7f) return;
        axis = axis * (1.f / len);
        const V3 ref = fabsf(axis.y) < 0.9f ? V3{ 0.f, 1.f, 0.f } : V3{ 1.f, 0.f, 0.f };
        const V3 u = Norm(Cross(axis, ref)), v = Cross(axis, u);
        const float sc = sqrtf(Dot(X.ax, X.ax));   // мм → мир
        const SCol col = PartColor(s.uPart);
        const V3 mid = (a + b) * 0.5f;
        for (int k = 0; k < n; k++)
        {
            const float t0 = 2.f * 3.14159265f * (float)k / (float)n, t1 = 2.f * 3.14159265f * (float)(k + 1) / (float)n;
            const V3 d0 = u * cosf(t0) + v * sinf(t0), d1 = u * cosf(t1) + v * sinf(t1);
            const V3 p0 = a + d0 * (s.ra * sc), p1 = a + d1 * (s.ra * sc), q0 = b + d0 * (s.rb * sc), q1 = b + d1 * (s.rb * sc);
            Tri(p0, p1, q1, mid, col);
            Tri(p0, q1, q0, mid, col);
        }
    }

    void Wing(const SXf& X, const SWing& w, float fAlpha, bool bVeins)
    {
        // Мембрана: вытянутый эллипс от шарнира назад (длина 2.1 мм, ширина 0.85 мм) и жилки
        const float L = 2.1f, Wd = 0.42f;
        const V3 hinge = P3(w.hinge);
        const auto P = [&](float x, float z) { return X(hinge + Rot(w.q, { x * w.fSide, 0.f, -z })); };
        const SCol mem = { 0.86f, 0.89f, 0.93f, 0.24f * fAlpha }, vein = { 0.32f, 0.30f, 0.28f, 0.55f * fAlpha };
        if (fAlpha <= 0.01f) return;
        const int n = 18;
        const V3 c0 = P(0.f, 0.f);
        for (int k = 0; k < n; k++)
        {
            const auto Edge = [&](int i)
            {
                const float t = 3.14159265f * (float)i / (float)n;          // 0..π по контуру
                const float z = L * 0.5f * (1.f - cosf(t));                 // от шарнира к кончику
                const float x = Wd * sinf(t) * (0.35f + 0.65f * sinf(t * 0.5f + 0.4f));
                return P(x, z);
            };
            Tri(c0, Edge(k), Edge(k + 1), c0, mem, 0.f, true);
            // нижняя кромка (задний край) — к средней линии
            const float t0 = 3.14159265f * (float)k / (float)n, t1 = 3.14159265f * (float)(k + 1) / (float)n;
            const V3 e0 = P(-0.12f * sinf(t0), L * 0.5f * (1.f - cosf(t0))), e1 = P(-0.12f * sinf(t1), L * 0.5f * (1.f - cosf(t1)));
            Tri(c0, e1, e0, c0, mem, 0.f, true);
        }
        // жилки: узкие полоски от шарнира веером
        if (!bVeins) return;
        const float aVein[4] = { 0.30f, 0.18f, 0.06f, -0.06f };
        for (float vx : aVein)
        {
            const V3 a0 = P(0.f, 0.03f), a1 = P(vx + 0.015f, L * 0.96f), a2 = P(vx - 0.015f, L * 0.96f);
            Tri(a0, a1, a2, a0, vein, 0.f, true);
        }
    }

    void Drop(const float d[4])
    {
        // Капля: купол из колец
        const V3 c = { d[0], d[1], d[2] };
        const float r = d[3];
        const int nLat = 5, nLon = 14;
        const SCol col = PartColor(kDrop);
        const auto Vt = [&](int i, int j)
        {
            const float th = 0.5f * 3.14159265f * (float)i / (float)nLat, ph = 2.f * 3.14159265f * (float)j / (float)nLon;
            return c + V3{ sinf(th) * cosf(ph) * r, cosf(th) * r * 0.6f, sinf(th) * sinf(ph) * r };
        };
        for (int i = 0; i < nLat; i++)
            for (int j = 0; j < nLon; j++)
            {
                if (i > 0) Tri(Vt(i, j), Vt(i, j + 1), Vt(i + 1, j + 1), c, col, 0.6f);
                Tri(Vt(i, j), Vt(i + 1, j + 1), Vt(i + 1, j), c, col, 0.6f);
            }
    }

    // ---------------------------------------------------------------- проекция
    constexpr float kNear = 0.01f;

    struct SEyeCtx
    {
        ImDrawList* dl;
        float x0, y0, sx, sy, tanL, tanU;
        ImVec2 uv;
        ImVec2 Project(V3 p) const { const float d = -p.z; return ImVec2(x0 + (p.x / d - tanL) * sx, y0 + (tanU - p.y / d) * sy); }

        void Poly(const V3* p, int n, ImU32 col) const
        {
            V3 o[8];
            int m = 0;
            for (int i = 0; i < n; i++)
            {
                const V3& a = p[i]; const V3& b = p[(i + 1) % n];
                const float da = -a.z - kNear, db = -b.z - kNear;
                if (da >= 0.f) o[m++] = a;
                if ((da >= 0.f) != (db >= 0.f)) o[m++] = a + (b - a) * (da / (da - db));
            }
            if (m < 3) return;
            dl->PrimReserve((m - 2) * 3, m);
            const ImDrawIdx base = (ImDrawIdx)dl->_VtxCurrentIdx;
            for (int i = 0; i < m; i++) dl->PrimWriteVtx(Project(o[i]), uv, col);
            for (int i = 1; i + 1 < m; i++) { dl->PrimWriteIdx(base); dl->PrimWriteIdx((ImDrawIdx)(base + i)); dl->PrimWriteIdx((ImDrawIdx)(base + i + 1)); }
        }
    };

    ImFont* g_pFont = nullptr;
}

void SetupWorldContext()
{
    // Шрифт подписи над мухой: Roboto из прошивки, иначе встроенный ImGui
    ImGuiIO& io = ImGui::GetIO();
    g_pFont = nullptr;
    for (const char* psz : { "/system/fonts/Roboto-Regular.ttf", "/system/fonts/RobotoStatic-Regular.ttf" })
        if (access(psz, R_OK) == 0 && (g_pFont = io.Fonts->AddFontFromFileTTF(psz, 30.f)) != nullptr) break;
    if (!g_pFont) g_pFont = io.Fonts->AddFontDefaultVector();
}

void DrawScene(ImDrawList* dl, const SScene& sc, const SWorldEye* pEyes, int nEyes)
{
    if (!sc.rig.bValid || nEyes <= 0) return;

    // ---- треугольники в мире
    g_World.clear();
    const auto Body = [&](V3 v) { return Rot(sc.aRot, v); };
    const float s = sc.fMmToWorld;
    const SXf X = { P3(sc.aOrigin), Body({ s, 0.f, 0.f }), Body({ 0.f, s, 0.f }), Body({ 0.f, 0.f, s }) };
    if (sc.bFly)
    {
        for (int i = 0; i < sc.pose.nEll; i++) Ellipsoid(X, sc.pose.aEll[i]);
        for (int i = 0; i < sc.pose.nSeg; i++) Segment(X, sc.pose.aSeg[i]);
        // в полёте крыло почти не видно — виден размытый веер взмаха
        for (const SWing& w : sc.pose.aWing) Wing(X, w, 1.f - 0.85f * sc.pose.fBlur, true);
        for (int i = 0; i < sc.pose.nBlur; i++) Wing(X, sc.pose.aBlur[i], 0.3f * sc.pose.fBlur, false);
    }
    for (int i = 0; i < sc.nDrops && i < 8; i++) Drop(sc.aDrops[i]);
    if (g_World.empty()) return;

    // ---- освещение (мир): свет сверху, мягкое заполнение снизу
    const V3 kLight = Norm({ 0.35f, 1.f, 0.25f });
    const SAffine toLayer = WorldToLayer(sc.rig);
    const ImVec2  uvWhite = ImGui::GetFontTexUvWhitePixel();

    for (int iEye = 0; iEye < nEyes; iEye++)
    {
        const SWorldEye& eye = pEyes[iEye];
        const SAffine    M   = Then(toLayer, LayerToEye(eye));
        SEyeCtx e;
        e.dl = dl; e.uv = uvWhite;
        e.tanL = eye.fTanL; e.tanU = eye.fTanU;
        e.x0 = eye.fX; e.y0 = eye.fY;
        e.sx = eye.fW / (eye.fTanR - eye.fTanL);
        e.sy = eye.fH / (eye.fTanU - eye.fTanD);

        // в оси глаза, без задних граней; порядок — от дальних к ближним
        g_Eye.clear();
        for (const STri& t : g_World)
        {
            STri v = t;
            for (int k = 0; k < 3; k++) v.p[k] = M.Point(t.p[k]);
            const V3 cen = (v.p[0] + v.p[1] + v.p[2]) * (1.f / 3.f);
            if (cen.z > 0.f && v.p[0].z > 0.f && v.p[1].z > 0.f && v.p[2].z > 0.f) continue;   // целиком за глазом
            const V3 nEye = M.Dir(t.n);
            if (!t.bTwoSided && Dot(nEye, cen) > 0.f) continue;
            v.fDepth = -cen.z;
            // свет: нормаль к глазу для двусторонних
            V3 n = t.n;
            if (t.bTwoSided && Dot(nEye, cen) > 0.f) n = -n;
            const float diff = fmaxf(Dot(n, kLight), 0.f);
            float k = 0.42f + 0.62f * diff;
            if (t.fSpec > 0.f) k += t.fSpec * powf(diff, 12.f);
            v.n = n;
            v.fSpec = k;   // коэффициент яркости для этого глаза
            g_Eye.push_back(v);
        }
        std::sort(g_Eye.begin(), g_Eye.end(), [](const STri& a, const STri& b) { return a.fDepth > b.fDepth; });

        dl->PushClipRect(ImVec2(eye.fX, eye.fY), ImVec2(eye.fX + eye.fW, eye.fY + eye.fH), false);
        for (const STri& t : g_Eye) e.Poly(t.p, 3, Shade(t.col, t.fSpec));

        // Издалека муха меньше пикселя и треугольники пропадают — как настоящая, видна тёмной точкой
        if (sc.bFly)
        {
            const V3 c = M.Point(X({ 0.f, 0.6f, 0.2f }));
            if (-c.z > kNear)
            {
                const float fPx = 1.25f * s / -c.z * e.sx;   // полдлины тела в пикселях
                if (fPx < 4.f)
                    dl->AddCircleFilled(e.Project(c), fmaxf(fPx * 0.6f, 1.1f), IM_COL32(20, 14, 10, (int)(230.f * Clamp01((4.f - fPx) / 2.f))), 8);
            }
        }

        // ---- подпись над мухой: плашка с текстом на глубине мухи
        if (sc.bFly && sc.szLabel[0] && g_pFont)
        {
            const V3 anchor = M.Point(P3(sc.aOrigin) + V3{ 0.f, 2.4f * s, 0.f });   // над мухой по вертикали мира
            if (-anchor.z > kNear)
            {
                const ImVec2 c  = e.Project(anchor);
                const float  fs = 30.f;
                const ImVec2 t1 = g_pFont->CalcTextSizeA(fs, FLT_MAX, 0.f, sc.szLabel);
                const ImVec2 t2 = sc.szLabel2[0] ? g_pFont->CalcTextSizeA(fs * 0.75f, FLT_MAX, 0.f, sc.szLabel2) : ImVec2(0.f, 0.f);
                const float  w  = fmaxf(t1.x, t2.x) + 36.f, h = t1.y + (sc.szLabel2[0] ? t2.y + 2.f : 0.f) + 16.f;
                const ImVec2 p0(c.x - w * 0.5f, c.y - h), p1(c.x + w * 0.5f, c.y);
                dl->AddRectFilled(p0, p1, IM_COL32(8, 8, 12, 190), h * 0.35f);
                dl->AddText(g_pFont, fs, ImVec2(c.x - t1.x * 0.5f, p0.y + 8.f), IM_COL32(255, 255, 255, 255), sc.szLabel);
                if (sc.szLabel2[0])
                    dl->AddText(g_pFont, fs * 0.75f, ImVec2(c.x - t2.x * 0.5f, p0.y + 10.f + t1.y), IM_COL32(150, 200, 255, 255), sc.szLabel2);
            }
        }
        dl->PopClipRect();
    }
}
}
