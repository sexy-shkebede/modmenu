#pragma once

#include <stdint.h>

// Тело мухи — кинематическая "кукла" (как FlyPoser в fly-connectome/scene.py, только без MuJoCo):
// шаги трипода, чистка передними лапами, хоботок, крылья, голова. Что делать, решает мозг
// (fly/sim, поведение — в mods/fly.cpp); здесь только как это выглядит.
//
// Оси мухи (мм, как в Unity): +X вправо, +Y вверх, +Z вперёд; начало — на полу под грудью.
// Муха дрозофилы: ~2.5 мм от головы до кончика брюшка.
namespace fly
{
    enum EPart : uint8_t { kHead, kEye, kThorax, kScutellum, kAbdomen, kLeg, kAntenna, kProboscis, kLabellum, kHaltere, kWing, kDrop, kPartCount };

    struct SEllipsoid { float c[3]; float r[3]; float q[4]; uint8_t uPart; };          // q — поворот (x, y, z, w)
    struct SSegment   { float a[3]; float b[3]; float ra, rb; uint8_t uPart; };        // конус от a к b
    struct SWing      { float hinge[3]; float q[4]; float fSide; };                    // плоскость крыла: длина вдоль −Z, ширина вдоль X

    struct SPose
    {
        SEllipsoid aEll[16];
        int        nEll = 0;
        SSegment   aSeg[40];
        int        nSeg = 0;
        SWing      aWing[2];
        float      fWingSpread = 0.f;
        // В полёте (200 взмахов в секунду) крыло не видно — видно размытый веер: положения
        // крыла по всему ходу взмаха; fBlur 0..1 — насколько крылья уже "размыты"
        SWing      aBlur[10];
        int        nBlur = 0;
        float      fBlur = 0.f;
    };

    // Команды телу на кадр — из поведения
    struct SBodyCmd
    {
        float fFwd       = 0.f;    // мм/с
        float fYaw       = 0.f;    // рад/с, + — влево
        bool  bGroom     = false;  // чистка передними лапами (DNg84/DNg35)
        bool  bEat       = false;  // стоит у еды
        float fProboscis = 0.f;    // 0..1 (MN9)
        bool  bFlight    = false;  // в полёте
        bool  bStartle   = false;  // вздрагивание
        float fHead      = 0.f;    // −1..1, + — голова влево (DNp15)
    };

    class CBody
    {
    public:
        void Update(float fDt, const SBodyCmd& cmd);   // fDt — время мухи, с
        void Pose(SPose& out) const;

    private:
        float m_fPhase = 0.f;          // фаза трипода, циклы
        float m_fFwd = 0.f, m_fYaw = 0.f;
        float m_fGroom = 0.f, m_fGroomT = 0.f;
        float m_fEat = 0.f, m_fProb = 0.f;
        float m_fFlight = 0.f, m_fFlapPhase = 0.f;
        float m_fStartle = 0.f;
        float m_fHead = 0.f;
        float m_fTime = 0.f;
    };
}
