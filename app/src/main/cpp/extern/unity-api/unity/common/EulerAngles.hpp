#pragma once

#include <math.h>
#include "Quaternion.hpp"

#define rad2deg(x) ((x) * (180.f / M_PI))
#define deg2rad(x) ((x) * (M_PI / 180.0))

struct EulerAngles
{
public:
    float pitch;
    float yaw;
    float roll;

    EulerAngles() = default;

    EulerAngles(float _pitch, float _yaw, float _roll)
    {
        this->pitch = _pitch;
        this->yaw   = _yaw;
        this->roll  = _roll;
    }
    ~EulerAngles() = default;

    EulerAngles operator+(EulerAngles vAngles)
    {
        return EulerAngles {
            this->pitch + vAngles.pitch,
            this->yaw   + vAngles.yaw,
            this->roll  + vAngles.roll
        };
    }

    EulerAngles operator+(float _f)
    {
        return EulerAngles {
            this->pitch + _f,
            this->yaw   + _f,
            this->roll  + _f
        };
    }

    EulerAngles& operator+=(EulerAngles vAngles)
    {
        *this = *this + vAngles;
        return *this;
    }

    EulerAngles& operator+=(float _f)
    {
        *this = *this + _f;
        return *this;
    }

    EulerAngles operator-(EulerAngles vAngles)
    {
        return EulerAngles {
            this->pitch - vAngles.pitch,
            this->yaw   - vAngles.yaw,
            this->roll  - vAngles.roll
        };
    }

    EulerAngles operator-(float _f)
    {
        return EulerAngles {
            this->pitch - _f,
            this->yaw   - _f,
            this->roll  - _f
        };
    }

    EulerAngles& operator-=(EulerAngles vAngles)
    {
        *this = *this - vAngles;
        return *this;
    }

    EulerAngles& operator-=(float _f)
    {
        *this = *this - _f;
        return *this;
    }

    EulerAngles operator*(EulerAngles vAngles)
    {
        return EulerAngles {
            this->pitch * vAngles.pitch,
            this->yaw   * vAngles.yaw,
            this->roll  * vAngles.roll
        };
    }

    EulerAngles operator*(float _f)
    {
        return EulerAngles {
            this->pitch * _f,
            this->yaw   * _f,
            this->roll  * _f,
        };
    }

    EulerAngles& operator*=(EulerAngles vAngles)
    {
        *this = *this * vAngles;
        return *this;
    }

    EulerAngles& operator*=(float _f)
    {
        *this = *this * _f;
        return *this;
    }

    EulerAngles operator/(EulerAngles vAngles)
    {
        return EulerAngles {
            this->pitch / vAngles.pitch,
            this->yaw   / vAngles.yaw,
            this->roll  / vAngles.roll
        };
    }

    EulerAngles operator/(float _f)
    {
        return EulerAngles {
            this->pitch / _f,
            this->yaw   / _f,
            this->roll  / _f
        };
    }

    EulerAngles& operator/=(EulerAngles vAngles)
    {
        *this = *this / vAngles;
        return *this;
    }

    EulerAngles& operator/=(float _f)
    {
        *this = *this / _f;
        return *this;
    }

    bool operator==(EulerAngles vAngles)
    {
        return this->pitch == vAngles.pitch && this->yaw == vAngles.yaw && this->roll == vAngles.roll;
    }

    bool operator!=(EulerAngles vAngles)
    {
        return !(*this == vAngles);
    }

    bool IsEmpty()
    {
        return !this->pitch && !this->yaw && !this->roll;
    }

    void ClampPitch(float fMaxPitch = 70.f)
    {
        if(this->pitch > fMaxPitch)
            this->pitch = fMaxPitch;
        if(this->pitch < -fMaxPitch)
            this->pitch = -fMaxPitch;
    }

    EulerAngles ClampedPitch(float fMaxPitch = 70.f)
    {
        EulerAngles ret = *this;
        ret.ClampPitch(fMaxPitch);
        return ret;
    }

    void normalize(float fMaxPitch = 90.f, bool bClampPitch = true)
    {
        if(bClampPitch)
            this->ClampPitch(fMaxPitch);
        else {
            if(this->pitch > fMaxPitch)
                this->pitch -= 360.f;
            if(this->pitch < -fMaxPitch)
                this->pitch += 360.f;
        }

        if(this->yaw > 180.f)
            this->yaw -= 360.f;
        if(this->yaw < -180.f)
            this->yaw += 360.f;

        this->roll = 0.f;
    }

    EulerAngles Normalized(float fMaxPitch = 90.f, bool bClampPitch = true)
    {
        EulerAngles ret = *this;
        ret.normalize(fMaxPitch, bClampPitch);
        return ret;
    }
};
