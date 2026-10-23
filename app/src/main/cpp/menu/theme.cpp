#include "theme.hpp"
#include "ui.hpp"

#include <math.h>

// ============================================================================
//  HCT = hue и chroma из CAM16 + tone (L* из CIELAB). Порт material-color-utilities.
// ============================================================================
namespace
{
    constexpr double kPi = 3.14159265358979323846;

    double Clamp(double v, double lo, double hi) { return v < lo ? lo : (v > hi ? hi : v); }
    double Sign (double v)                       { return v < 0 ? -1.0 : (v > 0 ? 1.0 : 0.0); }
    double SanitizeDeg(double d)                 { d = fmod(d, 360.0); return d < 0 ? d + 360.0 : d; }

    double Linearized(unsigned c)   // 0..255 → линейный 0..100
    {
        const double n = (double)c / 255.0;
        return (n <= 0.040449936 ? n / 12.92 : pow((n + 0.055) / 1.055, 2.4)) * 100.0;
    }
    unsigned Delinearized(double c) // линейный 0..100 → 0..255
    {
        const double n = c / 100.0;
        const double d = n <= 0.0031308 ? n * 12.92 : 1.055 * pow(n, 1.0 / 2.4) - 0.055;
        return (unsigned)Clamp(round(d * 255.0), 0.0, 255.0);
    }
    double LabF(double t)    { const double e = 216.0 / 24389.0, k = 24389.0 / 27.0; return t > e ? cbrt(t) : (k * t + 16.0) / 116.0; }
    double LabInvF(double f) { const double e = 216.0 / 24389.0, k = 24389.0 / 27.0; const double f3 = f * f * f; return f3 > e ? f3 : (116.0 * f - 16.0) / k; }
    double YFromLstar(double l) { return 100.0 * LabInvF((l + 16.0) / 116.0); }
    double LstarFromRgb(unsigned rgb)
    {
        const double y = 0.2126 * Linearized((rgb >> 16) & 0xFF) + 0.7152 * Linearized((rgb >> 8) & 0xFF) + 0.0722 * Linearized(rgb & 0xFF);
        return 116.0 * LabF(y / 100.0) - 16.0;
    }
    unsigned RgbFromLstar(double l) { const unsigned c = Delinearized(YFromLstar(l)); return (c << 16) | (c << 8) | c; }
    unsigned RgbFromXyz(double x, double y, double z)
    {
        const unsigned r = Delinearized( 3.2413774792388685 * x - 1.5376652402851851 * y - 0.49885366846268053 * z);
        const unsigned g = Delinearized(-0.9691452513005321 * x + 1.8758853451067872 * y + 0.04156585616912061 * z);
        const unsigned b = Delinearized( 0.05562093689691305 * x - 0.20395524564742123 * y + 1.0571799111220335 * z);
        return (r << 16) | (g << 8) | b;
    }

    // Стандартные условия просмотра sRGB (белая точка D65, фон L* 50, обычное окружение)
    struct SViewing { double n, aw, nbb, ncb, c, nc, rgbD[3], fl, flRoot, z; };
    const SViewing& Viewing()
    {
        static const SViewing vc = []
        {
            SViewing v{};
            const double wp[3] = { 95.047, 100.0, 108.883 };
            const double la    = (200.0 / kPi) * YFromLstar(50.0) / 100.0;
            const double rW = wp[0] *  0.401288 + wp[1] * 0.650173 + wp[2] * -0.051461;
            const double gW = wp[0] * -0.250268 + wp[1] * 1.204414 + wp[2] *  0.045854;
            const double bW = wp[0] * -0.002079 + wp[1] * 0.048952 + wp[2] *  0.953127;
            const double f  = 0.8 + 2.0 / 10.0;
            v.c  = f >= 0.9 ? 0.59 + (0.69 - 0.59) * ((f - 0.9) * 10.0) : 0.525 + (0.59 - 0.525) * ((f - 0.8) * 10.0);
            const double d = Clamp(f * (1.0 - (1.0 / 3.6) * exp((-la - 42.0) / 92.0)), 0.0, 1.0);
            v.nc = f;
            const double W[3] = { rW, gW, bW };
            for (int i = 0; i < 3; i++) v.rgbD[i] = d * (100.0 / W[i]) + 1.0 - d;
            const double k = 1.0 / (5.0 * la + 1.0), k4 = k * k * k * k, k4F = 1.0 - k4;
            v.fl     = k4 * la + 0.1 * k4F * k4F * cbrt(5.0 * la);
            v.flRoot = pow(v.fl, 0.25);
            v.n      = YFromLstar(50.0) / wp[1];
            v.z      = 1.48 + sqrt(v.n);
            v.nbb    = 0.725 / pow(v.n, 0.2);
            v.ncb    = v.nbb;
            double rgbA[3];
            for (int i = 0; i < 3; i++)
            {
                const double af = pow(v.fl * v.rgbD[i] * W[i] / 100.0, 0.42);
                rgbA[i] = 400.0 * af / (af + 27.13);
            }
            v.aw = (2.0 * rgbA[0] + rgbA[1] + 0.05 * rgbA[2]) * v.nbb;
            return v;
        }();
        return vc;
    }

    struct SCam { double hue, chroma, j, jstar, astar, bstar; };

    SCam CamFromJch(double j, double c, double h)
    {
        const SViewing& vc = Viewing();
        const double m     = c * vc.flRoot;
        const double hr    = h * kPi / 180.0;
        const double jstar = (1.0 + 100.0 * 0.007) * j / (1.0 + 0.007 * j);
        const double mstar = 1.0 / 0.0228 * log1p(0.0228 * m);
        return { h, c, j, jstar, mstar * cos(hr), mstar * sin(hr) };
    }

    SCam CamFromRgb(unsigned rgb)
    {
        const SViewing& vc = Viewing();
        const double r = Linearized((rgb >> 16) & 0xFF), g = Linearized((rgb >> 8) & 0xFF), b = Linearized(rgb & 0xFF);
        const double x = 0.41233895 * r + 0.35762064 * g + 0.18051042 * b;
        const double y = 0.2126     * r + 0.7152     * g + 0.0722     * b;
        const double z = 0.01932141 * r + 0.11916382 * g + 0.95034478 * b;
        const double rC =  0.401288 * x + 0.650173 * y - 0.051461 * z;
        const double gC = -0.250268 * x + 1.204414 * y + 0.045854 * z;
        const double bC = -0.002079 * x + 0.048952 * y + 0.953127 * z;
        const double D[3] = { vc.rgbD[0] * rC, vc.rgbD[1] * gC, vc.rgbD[2] * bC };
        double A[3];
        for (int i = 0; i < 3; i++)
        {
            const double af = pow(vc.fl * fabs(D[i]) / 100.0, 0.42);
            A[i] = Sign(D[i]) * 400.0 * af / (af + 27.13);
        }
        const double a  = (11.0 * A[0] - 12.0 * A[1] + A[2]) / 11.0;
        const double bb = (A[0] + A[1] - 2.0 * A[2]) / 9.0;
        const double u  = (20.0 * A[0] + 20.0 * A[1] + 21.0 * A[2]) / 20.0;
        const double p2 = (40.0 * A[0] + 20.0 * A[1] + A[2]) / 20.0;
        const double hue = SanitizeDeg(atan2(bb, a) * 180.0 / kPi);
        const double ac  = p2 * vc.nbb;
        const double j   = 100.0 * pow(ac / vc.aw, vc.c * vc.z);
        const double hp  = hue < 20.14 ? hue + 360.0 : hue;
        const double eHue = 0.25 * (cos(hp * kPi / 180.0 + 2.0) + 3.8);
        const double p1  = 50000.0 / 13.0 * eHue * vc.nc * vc.ncb;
        const double t   = p1 * hypot(a, bb) / (u + 0.305);
        const double alpha = pow(1.64 - pow(0.29, vc.n), 0.73) * pow(t, 0.9);
        const double c   = alpha * sqrt(j / 100.0);
        return CamFromJch(j, c, hue);
    }

    unsigned RgbFromCam(const SCam& cam)
    {
        const SViewing& vc = Viewing();
        const double alpha = (cam.chroma == 0.0 || cam.j == 0.0) ? 0.0 : cam.chroma / sqrt(cam.j / 100.0);
        const double t     = pow(alpha / pow(1.64 - pow(0.29, vc.n), 0.73), 1.0 / 0.9);
        const double hr    = cam.hue * kPi / 180.0;
        const double eHue  = 0.25 * (cos(hr + 2.0) + 3.8);
        const double ac    = vc.aw * pow(cam.j / 100.0, 1.0 / vc.c / vc.z);
        const double p1    = eHue * (50000.0 / 13.0) * vc.nc * vc.ncb;
        const double p2    = ac / vc.nbb;
        const double hs = sin(hr), hc = cos(hr);
        const double gamma = 23.0 * (p2 + 0.305) * t / (23.0 * p1 + 11.0 * t * hc + 108.0 * t * hs);
        const double a = gamma * hc, b = gamma * hs;
        const double A[3] = { (460.0 * p2 + 451.0 * a + 288.0 * b) / 1403.0,
                              (460.0 * p2 - 891.0 * a - 261.0 * b) / 1403.0,
                              (460.0 * p2 - 220.0 * a - 6300.0 * b) / 1403.0 };
        double F[3];
        for (int i = 0; i < 3; i++)
        {
            const double base = fmax(0.0, (27.13 * fabs(A[i])) / (400.0 - fabs(A[i])));
            F[i] = Sign(A[i]) * (100.0 / vc.fl) * pow(base, 1.0 / 0.42) / vc.rgbD[i];
        }
        return RgbFromXyz( 1.86206786 * F[0] - 1.01125463 * F[1] + 0.14918677 * F[2],
                           0.38752654 * F[0] + 0.62144744 * F[1] - 0.00897398 * F[2],
                          -0.01584150 * F[0] - 0.03412294 * F[1] + 1.04996444 * F[2]);
    }

    double CamDistance(const SCam& a, const SCam& b)
    {
        const double dj = a.jstar - b.jstar, da = a.astar - b.astar, db = a.bstar - b.bstar;
        return 1.41 * pow(sqrt(dj * dj + da * da + db * db), 0.63);
    }

    // Цвет с заданными hue/chroma и тоном: ищем J бисекцией, а если такой chroma
    // не помещается в sRGB — уменьшаем chroma (тоже бисекцией).
    bool FindByJ(double hue, double chroma, double tone, SCam& out)
    {
        double lo = 0.0, hi = 100.0, bestDL = 1000.0, bestDE = 1000.0;
        bool   bFound = false;
        while (fabs(lo - hi) > 0.01)
        {
            const double   mid     = lo + (hi - lo) / 2.0;
            const unsigned clipped = RgbFromCam(CamFromJch(mid, chroma, hue));
            const double   lstar   = LstarFromRgb(clipped);
            const double   dL      = fabs(tone - lstar);
            if (dL < 0.2)
            {
                const SCam   cam = CamFromRgb(clipped);
                const double dE  = CamDistance(cam, CamFromJch(cam.j, cam.chroma, hue));
                if (dE <= 1.0 && dE <= bestDE) { bestDL = dL; bestDE = dE; out = cam; bFound = true; }
            }
            if (bestDL == 0.0 && bestDE == 0.0) break;
            (lstar < tone ? lo : hi) = mid;
        }
        return bFound;
    }

    unsigned Solve(double hue, double chroma, double tone)
    {
        if (chroma < 1.0 || round(tone) <= 0.0 || round(tone) >= 100.0) return RgbFromLstar(tone);
        hue = SanitizeDeg(hue);
        double hi = chroma, mid = chroma, lo = 0.0;
        bool   bFirst = true, bAnswer = false;
        SCam   answer{}, cam{};
        while (fabs(lo - hi) >= 0.4)
        {
            const bool bOk = FindByJ(hue, mid, tone, cam);
            if (bFirst)
            {
                if (bOk) return RgbFromCam(cam);
                bFirst = false;
                mid = lo + (hi - lo) / 2.0;
                continue;
            }
            if (!bOk) hi = mid;
            else      { answer = cam; bAnswer = true; lo = mid; }
            mid = lo + (hi - lo) / 2.0;
        }
        return bAnswer ? RgbFromCam(answer) : RgbFromLstar(tone);
    }

    struct SPalette
    {
        double hue, chroma;
        unsigned Tone(double t) const { return Solve(hue, chroma, t); }
    };
    struct SPalettes { SPalette p, s, t, n, nv, err; };

    double RotatedHue(double h, const double* hues, const double* rot, int n)
    {
        for (int i = 0; i < n - 1; i++)
            if (hues[i] < h && h < hues[i + 1]) return SanitizeDeg(h + rot[i]);
        return h;
    }

    SPalettes MakePalettes(unsigned seed, theme::EStyle style)
    {
        const SCam     src = CamFromRgb(seed);
        const double   h = src.hue, c = src.chroma;
        // Палитра ошибок — от #B3261E, как в актуальной базовой схеме M3
        static const SCam kErr = CamFromRgb(0xB3261E);
        const SPalette    err  = { kErr.hue, kErr.chroma };
        switch (style)
        {
        case theme::EStyle::Vibrant:
        {
            static const double kHues[] = { 0, 41, 61, 101, 131, 181, 251, 301, 360 };
            static const double kSec[]  = { 18, 15, 10, 12, 15, 18, 15, 12, 12 };
            static const double kTer[]  = { 35, 30, 20, 25, 30, 35, 30, 25, 25 };
            return { { h, 200.0 }, { RotatedHue(h, kHues, kSec, 9), 24.0 }, { RotatedHue(h, kHues, kTer, 9), 32.0 }, { h, 10.0 }, { h, 12.0 }, err };
        }
        case theme::EStyle::Neutral:    return { { h, 12.0 }, { h, 8.0 }, { h, 16.0 }, { h, 2.0 }, { h, 2.0 }, err };
        case theme::EStyle::Monochrome: return { { h, 0.0 }, { h, 0.0 }, { h, 0.0 }, { h, 0.0 }, { h, 0.0 }, err };
        case theme::EStyle::TonalSpot:
        default:                        return { { h, fmax(48.0, c) }, { h, 16.0 }, { SanitizeDeg(h + 60.0), 24.0 }, { h, 6.0 }, { h, 8.0 }, err };
        }
    }

    // ---- Роли ui::role по порядку UI_COLOR_ROLES ----
    enum ERole
    {
    #define UI_ROLE_ENUM(name, rgb) k##name,
        UI_COLOR_ROLES(UI_ROLE_ENUM)
    #undef UI_ROLE_ENUM
        kRoleCount
    };
    unsigned* const s_apRoles[kRoleCount] =
    {
    #define UI_ROLE_PTR(name, rgb) &ui::role::name,
        UI_COLOR_ROLES(UI_ROLE_PTR)
    #undef UI_ROLE_PTR
    };

    unsigned s_aFrom[kRoleCount], s_aTo[kRoleCount];
    float    s_fAnim = 1.f;
}

void theme::Build(unsigned seed, EStyle style, bool bDark, unsigned* r)
{
    const SPalettes P = MakePalettes(seed, style);
    // Тоны ролей M3: тёмная схема / светлая схема
    const auto T = [&](const SPalette& pal, double dark, double light) { return pal.Tone(bDark ? dark : light); };
    r[kPrimary]                 = T(P.p,   80, 40);  r[kOnPrimary]            = T(P.p,   20, 100);
    r[kPrimaryContainer]        = T(P.p,   30, 90);  r[kOnPrimaryContainer]   = T(P.p,   90, 10);
    r[kSecondary]               = T(P.s,   80, 40);  r[kOnSecondary]          = T(P.s,   20, 100);
    r[kSecondaryContainer]      = T(P.s,   30, 90);  r[kOnSecondaryContainer] = T(P.s,   90, 10);
    r[kTertiary]                = T(P.t,   80, 40);  r[kOnTertiary]           = T(P.t,   20, 100);
    r[kTertiaryContainer]       = T(P.t,   30, 90);  r[kOnTertiaryContainer]  = T(P.t,   90, 10);
    r[kError]                   = T(P.err, 80, 40);  r[kOnError]              = T(P.err, 20, 100);
    r[kSurface]                 = T(P.n,    6, 98);
    r[kSurfaceContainerLowest]  = T(P.n,    4, 100);
    r[kSurfaceContainerLow]     = T(P.n,   10, 96);
    r[kSurfaceContainer]        = T(P.n,   12, 94);
    r[kSurfaceContainerHigh]    = T(P.n,   17, 92);
    r[kSurfaceContainerHighest] = T(P.n,   22, 90);
    r[kOnSurface]               = T(P.n,   90, 10);
    r[kOnSurfaceVariant]        = T(P.nv,  80, 30);
    r[kOutline]                 = T(P.nv,  60, 50);
    r[kOutlineVariant]          = T(P.nv,  30, 80);
    r[kInverseSurface]          = T(P.n,   90, 20);
    r[kInverseOnSurface]        = T(P.n,   20, 95);
}

void theme::Apply(unsigned seed, EStyle style, bool bDark, bool bAnimate)
{
    for (int i = 0; i < kRoleCount; i++) s_aFrom[i] = *s_apRoles[i];
    Build(seed, style, bDark, s_aTo);
    s_fAnim = bAnimate ? 0.f : 1.f;
    Tick(0.f);
}

void theme::Tick(float dt)
{
    if (s_fAnim >= 1.f && dt > 0.f) return;
    s_fAnim = s_fAnim + dt / ui::dur::Medium2;
    if (s_fAnim > 1.f) s_fAnim = 1.f;
    const float t = ui::EmphasizedDecelerate(s_fAnim);
    for (int i = 0; i < kRoleCount; i++) *s_apRoles[i] = ui::Mix(s_aFrom[i], s_aTo[i], t);
}

void theme::Preview(unsigned seed, EStyle style, bool bDark, unsigned out[3])
{
    const SPalettes P = MakePalettes(seed, style);
    out[0] = P.p.Tone(bDark ? 80 : 40);
    out[1] = P.s.Tone(bDark ? 60 : 70);
    out[2] = P.t.Tone(bDark ? 70 : 60);
}
