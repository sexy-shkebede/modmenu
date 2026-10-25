#include "body.hpp"

#include <math.h>

namespace
{
    struct V3 { float x, y, z; };
    V3    operator+(V3 a, V3 b)    { return { a.x + b.x, a.y + b.y, a.z + b.z }; }
    V3    operator-(V3 a, V3 b)    { return { a.x - b.x, a.y - b.y, a.z - b.z }; }
    V3    operator*(V3 a, float s) { return { a.x * s, a.y * s, a.z * s }; }
    float Dot(V3 a, V3 b)          { return a.x * b.x + a.y * b.y + a.z * b.z; }
    float Len(V3 a)                { return sqrtf(Dot(a, a)); }
    V3    Norm(V3 a)               { const float l = Len(a); return l > 1e-6f ? a * (1.f / l) : V3{ 0.f, 1.f, 0.f }; }
    V3    Lerp(V3 a, V3 b, float t){ return a + (b - a) * t; }
    float Clamp01(float t)         { return t < 0.f ? 0.f : (t > 1.f ? 1.f : t); }
    float Approach(float cur, float target, float rate, float dt) { return cur + (target - cur) * (1.f - expf(-rate * dt)); }

    struct Q4 { float x, y, z, w; };
    Q4 AxisAngle(V3 axis, float a) { const float s = sinf(a * 0.5f); return { axis.x * s, axis.y * s, axis.z * s, cosf(a * 0.5f) }; }
    Q4 Mul(Q4 a, Q4 b)
    {
        return { a.w * b.x + a.x * b.w + a.y * b.z - a.z * b.y,
                 a.w * b.y - a.x * b.z + a.y * b.w + a.z * b.x,
                 a.w * b.z + a.x * b.y - a.y * b.x + a.z * b.w,
                 a.w * b.w - a.x * b.x - a.y * b.y - a.z * b.z };
    }
    V3 Rot(Q4 q, V3 v)
    {
        const float x2 = q.x * 2.f, y2 = q.y * 2.f, z2 = q.z * 2.f;
        const float xx = q.x * x2, yy = q.y * y2, zz = q.z * z2, xy = q.x * y2, xz = q.x * z2, yz = q.y * z2;
        const float wx = q.w * x2, wy = q.w * y2, wz = q.w * z2;
        return { v.x * (1.f - (yy + zz)) + v.y * (xy - wz) + v.z * (xz + wy),
                 v.x * (xy + wz) + v.y * (1.f - (xx + zz)) + v.z * (yz - wx),
                 v.x * (xz - wy) + v.y * (yz + wx) + v.z * (1.f - (xx + yy)) };
    }
    const Q4 kIdent = { 0.f, 0.f, 0.f, 1.f };
    const V3 kX = { 1.f, 0.f, 0.f }, kY = { 0.f, 1.f, 0.f }, kZ = { 0.f, 0.f, 1.f };

    // ---- размеры (мм): ноги ----
    // Ноги по порядку: LF, LM, LH, RF, RM, RH. Тазобедренные суставы — снизу груди,
    // ступни в покое — передние вперёд, средние в стороны, задние назад (как у мухи).
    const V3   kHip[3]  = { { 0.15f, 0.40f, 0.62f }, { 0.20f, 0.38f, 0.38f }, { 0.20f, 0.38f, 0.14f } };
    const V3   kFoot[3] = { { 0.55f, 0.03f, 1.10f }, { 0.85f, 0.03f, 0.30f }, { 0.65f, 0.03f, -0.55f } };
    const float kFemur = 0.52f, kTibia = 0.50f, kTarsus = 0.40f;
    // Трипод: LF, RM, LH шагают вместе, RF, LM, RH — в противофазе
    const float kGaitOffset[6] = { 0.f, 0.5f, 0.f, 0.5f, 0.f, 0.5f };

    V3 Mirror(V3 v, float side) { return { v.x * side, v.y, v.z }; }

    // Две кости бедро–голень от конца тазика C до ступни F, колено — вверх
    V3 Knee(V3 c, V3 f)
    {
        V3    d   = f - c;
        float len = Len(d);
        const float maxLen = (kFemur + kTibia) * 0.995f;
        if (len > maxLen) { d = d * (maxLen / len); len = maxLen; }
        const V3    dir = d * (1.f / (len > 1e-5f ? len : 1e-5f));
        const float a   = (kFemur * kFemur - kTibia * kTibia + len * len) / (2.f * len);
        const float h   = sqrtf(fmaxf(kFemur * kFemur - a * a, 0.f));
        const V3    pole = Norm(kY - dir * Dot(kY, dir));
        return c + dir * a + pole * h;
    }

    // Чистка (как groom_angles в show.py): 0.6 с — лапы синхронно проводят по голове от морды
    // к макушке, 0.6 с — трут друг друга в противофазе; колебания 4 Гц
    V3 GroomFoot(float t, float side)
    {
        const float w = 2.f * 3.14159265f * 4.f;
        if (fmodf(t, 1.2f) < 0.6f)
        {
            const float a = 0.5f + 0.5f * sinf(w * t);
            return Lerp(V3{ 0.10f * side, 0.60f, 1.32f }, V3{ 0.15f * side, 0.95f, 1.08f }, a);
        }
        const float o = sinf(w * 1.5f * t + (side > 0.f ? 3.14159265f : 0.f));
        return { side * (0.06f + 0.05f * o), 0.38f + 0.04f * o, 1.24f + 0.04f * o };
    }
}

void fly::CBody::Update(float fDt, const SBodyCmd& c)
{
    if (fDt <= 0.f) return;
    m_fTime += fDt;
    const bool bStand = c.bGroom || c.bFlight;   // у еды стоит поведение (fFwd = 0), но может подползти
    m_fFwd = Approach(m_fFwd, bStand ? 0.f : c.fFwd, 12.f, fDt);
    m_fYaw = Approach(m_fYaw, bStand ? 0.f : c.fYaw, 12.f, fDt);

    // Частота шагов — по скорости ступней: шаг ~1 мм, до 12 Гц (живая муха — около 10 Гц)
    const float fSpeed = fabsf(m_fFwd) + fabsf(m_fYaw) * 0.7f;
    const float fFreq  = fminf(fSpeed / 1.0f, 12.f);
    m_fPhase = fmodf(m_fPhase + fFreq * fDt, 1.f);

    m_fGroom = Approach(m_fGroom, c.bGroom ? 1.f : 0.f, 10.f, fDt);
    m_fGroomT = c.bGroom ? m_fGroomT + fDt : 0.f;
    m_fEat   = Approach(m_fEat, c.bEat ? 1.f : 0.f, 8.f, fDt);
    m_fProb  = Approach(m_fProb, Clamp01(c.fProboscis), 9.f, fDt);   // MN9 выдвигает хоботок и без еды
    m_fFlight = Approach(m_fFlight, c.bFlight ? 1.f : 0.f, c.bFlight ? 25.f : 6.f, fDt);
    m_fStartle = c.bStartle ? 1.f : Approach(m_fStartle, 0.f, 10.f, fDt);
    m_fHead  = Approach(m_fHead, fmaxf(-1.f, fminf(1.f, c.fHead)), 8.f, fDt);
    // Взмахи: в полёте быстро (живая муха — 200 Гц, тут медленнее, иначе крылья "стоят"), при вздрагивании — пара взмахов
    const float fFlapHz = 14.f * fmaxf(m_fFlight, m_fStartle);
    m_fFlapPhase = fmodf(m_fFlapPhase + fFlapHz * fDt, 1.f);
}

void fly::CBody::Pose(SPose& o) const
{
    o.nEll = 0;
    o.nSeg = 0;
    const auto Ell = [&](V3 c, V3 r, Q4 q, EPart part)
    {
        SEllipsoid& e = o.aEll[o.nEll++];
        e = { { c.x, c.y, c.z }, { r.x, r.y, r.z }, { q.x, q.y, q.z, q.w }, part };
    };
    const auto Seg = [&](V3 a, V3 b, float ra, float rb, EPart part)
    {
        SSegment& s = o.aSeg[o.nSeg++];
        s = { { a.x, a.y, a.z }, { b.x, b.y, b.z }, ra, rb, part };
    };

    // ---- корпус: грудь чуть задрана, брюшко опущено к кончику; при чистке — привстаёт
    const float fPitch = -0.14f - 0.10f * m_fGroom;
    Ell({ 0.f, 0.62f, 0.35f }, { 0.40f, 0.38f, 0.50f }, AxisAngle(kX, fPitch), kThorax);
    Ell({ 0.f, 0.86f, -0.05f }, { 0.18f, 0.08f, 0.15f }, kIdent, kScutellum);
    Ell({ 0.f, 0.52f, -0.62f }, { 0.38f, 0.33f, 0.70f }, AxisAngle(kX, 0.17f), kAbdomen);
    Ell({ 0.22f, 0.70f, -0.05f }, { 0.05f, 0.05f, 0.05f }, kIdent, kHaltere);
    Ell({ -0.22f, 0.70f, -0.05f }, { 0.05f, 0.05f, 0.05f }, kIdent, kHaltere);

    // ---- голова: поворот вокруг шеи (DNp15), у еды — наклон вниз
    const V3 neck = { 0.f, 0.62f, 0.82f };
    const Q4 qHead = Mul(AxisAngle(kY, -0.45f * m_fHead), AxisAngle(kX, 0.25f * m_fEat + 0.08f * m_fGroom));
    const auto H = [&](V3 p) { return neck + Rot(qHead, p - neck); };
    Ell(H({ 0.f, 0.66f, 1.02f }), { 0.30f, 0.27f, 0.20f }, qHead, kHead);
    for (int s = -1; s <= 1; s += 2)
    {
        const float fs = (float)s;
        Ell(H({ 0.25f * fs, 0.68f, 1.05f }), { 0.13f, 0.24f, 0.20f }, Mul(qHead, AxisAngle(kY, 0.26f * fs)), kEye);
        const V3 a0 = H({ 0.06f * fs, 0.74f, 1.20f }), a1 = H({ 0.08f * fs, 0.80f, 1.32f }), a2 = H({ 0.15f * fs, 0.93f, 1.42f });
        Seg(a0, a1, 0.035f, 0.045f, kAntenna);
        Seg(a1, a2, 0.008f, 0.004f, kAntenna);                 // ариста
    }
    // хоботок (MN9): выдвигается вниз-вперёд, на конце — лабеллум
    const V3 pb  = H({ 0.f, 0.50f, 1.10f });
    const V3 dir = Rot(qHead, Norm({ 0.f, -0.85f, 0.5f }));
    const V3 pt  = pb + dir * (0.06f + 0.40f * m_fProb);
    Seg(pb, pt, 0.06f, 0.05f, kProboscis);
    Ell(pt, V3{ 0.08f, 0.06f, 0.08f } * (0.6f + 0.4f * m_fProb), qHead, kLabellum);

    // ---- ноги
    const float fStride = 1.f / fmaxf(fminf((fabsf(m_fFwd) + fabsf(m_fYaw) * 0.7f) / 1.0f, 12.f), 0.5f);   // с на цикл
    const float fWalk   = Clamp01((fabsf(m_fFwd) + fabsf(m_fYaw) * 0.7f) / 0.5f) * (1.f - m_fGroom) * (1.f - m_fFlight);
    for (int leg = 0; leg < 6; leg++)
    {
        const int   k    = leg % 3;
        const float side = leg < 3 ? -1.f : 1.f;
        const V3    hip  = Mirror(kHip[k], side);
        const V3    rest = Mirror(kFoot[k], side);

        // Шаг: за цикл тело уходит на D = (v + ω × r) · T; в опоре ступня стоит в мире
        // точка тела над ступнёй: при повороте влево передние уходят влево, левые — назад
        const V3    vel  = { -m_fYaw * rest.z, 0.f, m_fFwd + m_fYaw * rest.x };
        const V3    D    = vel * fStride * fWalk;
        const float s    = fmodf(m_fPhase + kGaitOffset[leg], 1.f);
        V3 foot;
        if (s < 0.5f) foot = rest + D * (0.25f - s);
        else
        {
            const float u = (s - 0.5f) * 2.f;
            foot = rest + D * (-0.25f + 0.5f * u) + kY * (0.22f * sinf(3.14159265f * u) * fWalk);
        }
        // Чистка передними лапами
        if (k == 0 && m_fGroom > 0.001f) foot = Lerp(foot, GroomFoot(m_fGroomT, side), m_fGroom);
        // В полёте лапы поджаты
        if (m_fFlight > 0.001f) foot = Lerp(foot, hip + V3{ side * 0.22f, -0.28f, (float)(1 - k) * 0.25f }, m_fFlight);

        const V3 h   = Norm({ foot.x - hip.x, 0.f, foot.z - hip.z });
        const V3 cx  = hip + h * 0.12f + V3{ 0.f, -0.16f, 0.f };   // конец тазика
        const V3 kn  = Knee(cx, foot);
        const V3 tip = foot + Norm(h * 1.f + V3{ 0.f, -0.25f, 0.f }) * kTarsus;
        Seg(hip, cx, 0.07f, 0.06f, kLeg);
        Seg(cx, kn, 0.055f, 0.045f, kLeg);
        Seg(kn, foot, 0.04f, 0.03f, kLeg);
        Seg(foot, tip, 0.025f, 0.018f, kLeg);
    }

    // ---- крылья: в покое сложены над брюшком, в полёте раскрыты и машут
    const float fSpread = fmaxf(m_fFlight, 0.7f * m_fStartle);
    const float fStroke = 1.1f * sinf(2.f * 3.14159265f * m_fFlapPhase) * fSpread;
    for (int i = 0; i < 2; i++)
    {
        const float side = i == 0 ? -1.f : 1.f;
        const Q4 qOut  = AxisAngle(kY, side * -(0.22f + 1.25f * fSpread));            // от средней линии наружу
        const Q4 qFlap = AxisAngle(kZ, side * (fStroke + 0.08f));                       // взмах вверх-вниз
        const Q4 q     = Mul(qFlap, Mul(qOut, AxisAngle(kX, -0.05f)));
        SWing& w = o.aWing[i];
        w = { { 0.16f * side, 0.86f, 0.40f }, { q.x, q.y, q.z, q.w }, side };
    }
    o.fWingSpread = fSpread;
}
