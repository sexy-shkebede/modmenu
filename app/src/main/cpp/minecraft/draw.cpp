#include "draw.hpp"
#include "blocks.hpp"
#include "../openxr_hook/world_view.hpp"

#include "imgui_internal.h"   // ImClamp

#include <algorithm>
#include <math.h>

namespace mc
{
namespace
{
    struct V3 { float x, y, z; };
    V3    operator+(V3 a, V3 b)    { return { a.x + b.x, a.y + b.y, a.z + b.z }; }
    V3    operator-(V3 a, V3 b)    { return { a.x - b.x, a.y - b.y, a.z - b.z }; }
    V3    operator*(V3 a, float s) { return { a.x * s, a.y * s, a.z * s }; }
    V3    operator-(V3 a)          { return { -a.x, -a.y, -a.z }; }
    float Dot(V3 a, V3 b)          { return a.x * b.x + a.y * b.y + a.z * b.z; }

    V3 Rotate(const float q[4], V3 v)
    {
        const float x2 = q[0] * 2.f, y2 = q[1] * 2.f, z2 = q[2] * 2.f;
        const float xx = q[0] * x2, yy = q[1] * y2, zz = q[2] * z2;
        const float xy = q[0] * y2, xz = q[0] * z2, yz = q[1] * z2;
        const float wx = q[3] * x2, wy = q[3] * y2, wz = q[3] * z2;
        return { v.x * (1.f - (yy + zz)) + v.y * (xy - wz)         + v.z * (xz + wy),
                 v.x * (xy + wz)         + v.y * (1.f - (xx + zz)) + v.z * (yz - wx),
                 v.x * (xz - wy)         + v.y * (yz + wx)         + v.z * (1.f - (xx + yy)) };
    }

    // Аффинное преобразование: p' = ax * p.x + ay * p.y + az * p.z + t
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

    // Мир Unity → пространство трекинга игры (OpenXR): обратный риг и смена руки (Z → -Z)
    SAffine WorldToLayer(const SRig& rig)
    {
        const float qInv[4] = { -rig.aRot[0], -rig.aRot[1], -rig.aRot[2], rig.aRot[3] };
        const float s       = rig.fScale > 1e-4f ? 1.f / rig.fScale : 1.f;
        const auto  Axis    = [&](V3 e) { const V3 r = Rotate(qInv, e) * s; return V3{ r.x, r.y, -r.z }; };
        SAffine a = { Axis({ 1.f, 0.f, 0.f }), Axis({ 0.f, 1.f, 0.f }), Axis({ 0.f, 0.f, 1.f }), { 0.f, 0.f, 0.f } };
        a.t = -a.Dir({ rig.aPos[0], rig.aPos[1], rig.aPos[2] });
        return a;
    }

    // Пространство слоя → оси глаза (OpenXR: -Z вперёд)
    SAffine LayerToEye(const SWorldEye& e)
    {
        const float qInv[4] = { -e.aRot[0], -e.aRot[1], -e.aRot[2], e.aRot[3] };
        SAffine a = { Rotate(qInv, { 1.f, 0.f, 0.f }), Rotate(qInv, { 0.f, 1.f, 0.f }), Rotate(qInv, { 0.f, 0.f, 1.f }), { 0.f, 0.f, 0.f } };
        a.t = -a.Dir({ e.aPos[0], e.aPos[1], e.aPos[2] });
        return a;
    }

    // Грани в осях Unity: нормаль, направление текстуры вправо (u) и вниз (v), как смотреть снаружи
    struct SFaceAxes { int n, u, v; };   // (номер оси + 1) со знаком: +1 = +X, -2 = -Y, +3 = +Z
    constexpr SFaceAxes kFaces[kFaceCount] =
    {
        { +1, +3, -2 },   // +X: u = +Z, v = -Y
        { -1, -3, -2 },   // -X: u = -Z, v = -Y
        { +2, +1, -3 },   // +Y: u = +X, v = -Z
        { -2, -1, -3 },   // -Y: u = -X, v = -Z
        { +3, -1, -2 },   // +Z: u = -X, v = -Y
        { -3, +1, -2 },   // -Z: u = +X, v = -Y
    };

    // Затенение граней как в Minecraft (множители sRGB, переведённые в линейные)
    float ShadeOf(int iFace)
    {
        static const float kSrgb[kFaceCount] = { 0.6f, 0.6f, 1.f, 0.5f, 0.8f, 0.8f };
        return powf(kSrgb[iFace], 2.2f);
    }

    ImU32 Gray(float fLinear, float fAlpha)
    {
        const int g = (int)(ImClamp(fLinear, 0.f, 1.f) * 255.f + 0.5f);
        const int a = (int)(ImClamp(fAlpha, 0.f, 1.f) * 255.f + 0.5f);
        return IM_COL32(g, g, g, a);
    }

    constexpr float kNear     = 0.03f;     // м: ближе к глазу грани обрезаются
    constexpr float kAffine   = 0.08f;     // допустимый перепад глубины на кусок грани (ImGui мапит UV без перспективы)
    constexpr int   kMaxSplit = 6;
    constexpr int   kMaxVerts = 600000;    // предохранитель на кадр

    struct SEyeCtx
    {
        ImDrawList* dl;
        SAffine     toEye;   // мир Unity → глаз
        float       x0, y0, sx, sy, tanL, tanU, tanR, tanD;

        ImVec2 Project(V3 p) const
        {
            const float d = -p.z;
            return ImVec2(x0 + (p.x / d - tanL) * sx, y0 + (tanU - p.y / d) * sy);
        }

        // Выпуклый многоугольник (все точки перед ближней плоскостью) — веером
        void Emit(const V3* p, const ImVec2* uv, int n, ImU32 col) const
        {
            if (n < 3) return;
            dl->PrimReserve((n - 2) * 3, n);
            const ImDrawIdx base = (ImDrawIdx)dl->_VtxCurrentIdx;
            for (int i = 0; i < n; i++) dl->PrimWriteVtx(Project(p[i]), uv[i], col);
            for (int i = 1; i + 1 < n; i++)
            {
                dl->PrimWriteIdx(base);
                dl->PrimWriteIdx((ImDrawIdx)(base + i));
                dl->PrimWriteIdx((ImDrawIdx)(base + i + 1));
            }
        }

        // Четырёхугольник с обрезкой по ближней плоскости (Сазерленд — Ходжман)
        void Quad(const V3 p[4], const ImVec2 uv[4], ImU32 col) const
        {
            if (-p[0].z >= kNear && -p[1].z >= kNear && -p[2].z >= kNear && -p[3].z >= kNear)
            {
                Emit(p, uv, 4, col);
                return;
            }
            V3     op[8];
            ImVec2 ouv[8];
            int    m = 0;
            for (int i = 0; i < 4; i++)
            {
                const int   j  = (i + 1) & 3;
                const float da = -p[i].z - kNear, db = -p[j].z - kNear;
                if (da >= 0.f) { op[m] = p[i]; ouv[m] = uv[i]; m++; }
                if ((da >= 0.f) != (db >= 0.f))
                {
                    const float t = da / (da - db);
                    op[m]  = p[i] + (p[j] - p[i]) * t;
                    ouv[m] = ImVec2(uv[i].x + (uv[j].x - uv[i].x) * t, uv[i].y + (uv[j].y - uv[i].y) * t);
                    m++;
                }
            }
            Emit(op, ouv, m, col);
        }

        // Отрезок с обрезкой по ближней плоскости
        void Line(V3 a, V3 b, ImU32 col, float fThickness) const
        {
            const float da = -a.z - kNear, db = -b.z - kNear;
            if (da < 0.f && db < 0.f) return;
            if (da < 0.f) a = a + (b - a) * (da / (da - db));
            if (db < 0.f) b = b + (a - b) * (db / (db - da));
            dl->AddLine(Project(a), Project(b), col, fThickness);
        }
    };

    // Куб в осях глаза: центр и полуоси (мировые X, Y, Z блока)
    struct SCube
    {
        V3 c, a[3];
        V3 Axis(int iSigned) const { return iSigned > 0 ? a[iSigned - 1] : -a[-iSigned - 1]; }
        bool Front(int iFace) const
        {
            const V3 n = Axis(kFaces[iFace].n);
            return Dot(n, c + n) < 0.f;   // нормаль смотрит на глаз (он в начале координат)
        }
    };

    SCube MakeCube(const SEyeCtx& e, const float c[3], float fHalf)
    {
        return { e.toEye.Point({ c[0], c[1], c[2] }), { e.toEye.ax * fHalf, e.toEye.ay * fHalf, e.toEye.az * fHalf } };
    }

    // Невидим целиком: шар вокруг куба за ближней плоскостью или вне поля зрения
    bool Culled(const SEyeCtx& e, const SCube& k, float fHalf)
    {
        const float r = fHalf * 1.7321f * 1.05f;
        const float d = -k.c.z;
        if (d + r < kNear) return true;
        const auto Out = [&](float fSigned, float fTan) { return fSigned / sqrtf(1.f + fTan * fTan) < -r; };
        return Out(k.c.x - e.tanL * d, e.tanL) || Out(e.tanR * d - k.c.x, e.tanR) ||
               Out(e.tanU * d - k.c.y, e.tanU) || Out(k.c.y - e.tanD * d, e.tanD);
    }

    void DrawFace(const SEyeCtx& e, const SCube& k, const SBlockType& type, int iFace, int iAxis, int iFacing, float fBright, float fAlpha)
    {
        bool           bRotate = false;
        const uint16_t uTile   = FaceTile(type, iFace, iAxis, iFacing, bRotate);
        const SUv      uv      = TileUv(uTile);
        const V3       n       = k.Axis(kFaces[iFace].n);
        V3             u       = k.Axis(kFaces[iFace].u);
        V3             v       = k.Axis(kFaces[iFace].v);
        if (bRotate) { const V3 t = u; u = v; v = -t; }   // текстура на 90°

        const V3 fc = k.c + n;
        const V3 corner[4] = { fc - u - v, fc + u - v, fc + u + v, fc - u + v };   // по UV: (0,0) (1,0) (1,1) (0,1)
        float dMin = 1e9f, dMax = -1e9f;
        for (const V3& p : corner) { dMin = fminf(dMin, -p.z); dMax = fmaxf(dMax, -p.z); }

        // ImGui интерполирует UV линейно по экрану: глубокую грань дробим, пока искажение незаметно
        int nSplit = kMaxSplit;
        if (dMin > kNear)
            nSplit = ImClamp((int)ceilf((dMax - dMin) / (kAffine * dMin)), 1, kMaxSplit);

        const float  fShade = (type.uFlags & kGlow) ? 1.f : ShadeOf(iFace);
        const ImU32  col    = Gray(fShade * fBright, fAlpha);
        const float  inv    = 1.f / (float)nSplit;
        for (int j = 0; j < nSplit; j++)
            for (int i = 0; i < nSplit; i++)
            {
                const float s0 = (float)i * inv, s1 = (float)(i + 1) * inv;
                const float t0 = (float)j * inv, t1 = (float)(j + 1) * inv;
                const auto  P  = [&](float s, float t) { return fc + u * (s * 2.f - 1.f) + v * (t * 2.f - 1.f); };
                const auto  T  = [&](float s, float t) { return ImVec2(uv.u0 + (uv.u1 - uv.u0) * s, uv.v0 + (uv.v1 - uv.v0) * t); };
                const V3     p[4]  = { P(s0, t0), P(s1, t0), P(s1, t1), P(s0, t1) };
                const ImVec2 uvs[4] = { T(s0, t0), T(s1, t0), T(s1, t1), T(s0, t1) };
                e.Quad(p, uvs, col);
            }
    }

    void DrawBlock(const SEyeCtx& e, const SCube& k, int iType, int iAxis, int iFacing, uint8_t uHidden, float fBright, float fAlpha)
    {
        const SBlockType& type = Block(iType);
        bool abFront[kFaceCount];
        for (int f = 0; f < kFaceCount; f++) abFront[f] = k.Front(f);

        // Сквозь прозрачные пиксели (стекло, листва, лёд) видны задние грани — их первыми
        if (type.uFlags & kTranslucent)
            for (int f = 0; f < kFaceCount; f++)
                if (!abFront[f] && !(uHidden & (1u << f)))
                    DrawFace(e, k, type, f, iAxis, iFacing, fBright, fAlpha);
        for (int f = 0; f < kFaceCount; f++)
            if (abFront[f] && !(uHidden & (1u << f)))
                DrawFace(e, k, type, f, iAxis, iFacing, fBright, fAlpha);
    }

    // Рамка куба: только рёбра, у которых видна хотя бы одна грань
    void DrawOutline(const SEyeCtx& e, const SCube& k, ImU32 colInner)
    {
        bool abFront[kFaceCount];
        for (int f = 0; f < kFaceCount; f++) abFront[f] = k.Front(f);
        const auto Corner = [&](int i) { return k.c + k.a[0] * ((i & 1) ? 1.f : -1.f) + k.a[1] * ((i & 2) ? 1.f : -1.f) + k.a[2] * ((i & 4) ? 1.f : -1.f); };
        const auto FaceOf = [](int iAxis, bool bPositive) { return iAxis * 2 + (bPositive ? 0 : 1); };

        for (int iPass = 0; iPass < 2; iPass++)
            for (int i = 0; i < 8; i++)
                for (int iAxis = 0; iAxis < 3; iAxis++)
                {
                    const int bit = 1 << iAxis;
                    if (i & bit) continue;                       // каждое ребро — один раз, от "минусового" конца
                    const int a1 = (iAxis + 1) % 3, a2 = (iAxis + 2) % 3;
                    if (!abFront[FaceOf(a1, (i >> a1) & 1)] && !abFront[FaceOf(a2, (i >> a2) & 1)]) continue;
                    if (iPass == 0) e.Line(Corner(i), Corner(i | bit), IM_COL32(0, 0, 0, 110), 5.f);
                    else            e.Line(Corner(i), Corner(i | bit), colInner, 2.f);
                }
    }
}

void SetSamplerNearest(ImDrawList* dl, bool bNearest)
{
    const ImGuiPlatformIO& pio = ImGui::GetPlatformIO();
    const ImDrawCallback   cb  = bNearest ? pio.DrawCallback_SetSamplerNearest : pio.DrawCallback_SetSamplerLinear;
    if (cb) dl->AddCallback(cb, nullptr);
}

void DrawIcon(ImDrawList* dl, ImTextureRef atlas, int iType, ImVec2 c, float fSize, float fAlpha)
{
    const SBlockType& type = Block(iType);
    const float a  = fAlpha * ImGui::GetStyle().Alpha;
    const float hw = fSize * 0.5f * 0.8660f, q = fSize * 0.25f;
    const ImVec2 top(c.x, c.y - fSize * 0.5f), ul(c.x - hw, c.y - q), ur(c.x + hw, c.y - q);
    const ImVec2 lo(c.x, c.y + fSize * 0.5f), ll(c.x - hw, c.y + q), lr(c.x + hw, c.y + q);
    const bool   bGlow = (type.uFlags & kGlow) != 0;

    // Слева — лицевая сторона (-Z), справа — бок (+X), сверху — верх; колонна стоит вертикально
    bool bRotate = false;
    const SUv t = TileUv(FaceTile(type, kPosY, 1, kNegZ, bRotate));
    const SUv l = TileUv(FaceTile(type, kNegZ, 1, kNegZ, bRotate));
    const SUv r = TileUv(FaceTile(type, kPosX, 1, kNegZ, bRotate));
    dl->PushTexture(atlas);
    dl->PrimReserve(18, 12);
    dl->PrimQuadUV(ul, top, ur, c, ImVec2(t.u0, t.v0), ImVec2(t.u1, t.v0), ImVec2(t.u1, t.v1), ImVec2(t.u0, t.v1), Gray(1.f, a));
    dl->PrimQuadUV(ul, c, lo, ll,  ImVec2(l.u0, l.v0), ImVec2(l.u1, l.v0), ImVec2(l.u1, l.v1), ImVec2(l.u0, l.v1), Gray(bGlow ? 1.f : ShadeOf(kNegZ), a));
    dl->PrimQuadUV(c, ur, lr, lo,  ImVec2(r.u0, r.v0), ImVec2(r.u1, r.v0), ImVec2(r.u1, r.v1), ImVec2(r.u0, r.v1), Gray(bGlow ? 1.f : ShadeOf(kPosX), a));
    dl->PopTexture();
}

void DrawWorld(ImDrawList* dl, ImTextureRef atlas, const SWorld& world, const SWorldEye* pEyes, int nEyes)
{
    if (!world.rig.bValid || nEyes <= 0) return;
    const SAffine toLayer = WorldToLayer(world.rig);

    // Порядок "от дальних к ближним" — один на оба глаза (от середины между ними),
    // чтобы глаза не спорили, какой блок впереди
    V3 mid = { 0.f, 0.f, 0.f };
    for (int i = 0; i < nEyes; i++) mid = mid + V3{ pEyes[i].aPos[0], pEyes[i].aPos[1], pEyes[i].aPos[2] } * (1.f / (float)nEyes);
    static std::vector<std::pair<float, int>> s_Order;
    s_Order.clear();
    for (int i = 0; i < (int)world.blocks.size(); i++)
    {
        const SPlaced& b = world.blocks[i];
        if (b.fAlpha < 0.01f) continue;
        const V3 d = toLayer.Point({ b.c[0], b.c[1], b.c[2] }) - mid;
        s_Order.push_back({ Dot(d, d), i });
    }
    std::sort(s_Order.begin(), s_Order.end(), [](const std::pair<float, int>& a, const std::pair<float, int>& b) { return a.first > b.first; });

    const SCursor& cur = world.cursor;
    for (int iEye = 0; iEye < nEyes; iEye++)
    {
        const SWorldEye& eye = pEyes[iEye];
        SEyeCtx e;
        e.dl    = dl;
        e.toEye = Then(toLayer, LayerToEye(eye));
        e.tanL  = eye.fTanL; e.tanR = eye.fTanR; e.tanU = eye.fTanU; e.tanD = eye.fTanD;
        e.x0    = eye.fX;    e.y0   = eye.fY;
        e.sx    = eye.fW / (eye.fTanR - eye.fTanL);
        e.sy    = eye.fH / (eye.fTanU - eye.fTanD);

        dl->PushClipRect(ImVec2(eye.fX, eye.fY), ImVec2(eye.fX + eye.fW, eye.fY + eye.fH), false);
        SetSamplerNearest(dl, true);
        dl->PushTexture(atlas);
        for (const std::pair<float, int>& o : s_Order)
        {
            if (dl->VtxBuffer.Size > kMaxVerts) break;
            const SPlaced& b    = world.blocks[o.second];
            const float    half = b.fHalf * (0.75f + 0.25f * b.fScale);   // блок "вырастает" при установке
            const SCube    k    = MakeCube(e, b.c, half);
            if (Culled(e, k, half)) continue;
            DrawBlock(e, k, b.uType, b.uAxis, b.uFacing, b.uHiddenFaces, world.fBrightness, b.fAlpha);
        }

        // Призрак будущего блока — полупрозрачный, поверх остальных
        const SCube kCur = MakeCube(e, cur.c, cur.fHalf * 1.004f);
        if (cur.bShow && cur.bGhost && !Culled(e, kCur, cur.fHalf))
            DrawBlock(e, kCur, cur.uType, cur.uAxis, cur.uFacing, 0, world.fBrightness, 0.45f);
        dl->PopTexture();
        SetSamplerNearest(dl, false);

        // Рамка: где появится блок или какой сломается
        if (cur.bShow && !Culled(e, kCur, cur.fHalf))
            DrawOutline(e, kCur, cur.bGhost ? IM_COL32(255, 255, 255, 230) : IM_COL32(255, 96, 96, 240));
        dl->PopClipRect();
    }
}
}
