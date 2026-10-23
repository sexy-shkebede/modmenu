#pragma once

#include <math.h>
#include "EulerAngles.hpp"

struct Quaternion {
public:
    float x;
    float y;
    float z;
    float w;

    Quaternion() = default;
    Quaternion(float _x, float _y, float _z, float _w)
    {
        this->x = _x;
        this->y = _y;
        this->z = _z;
        this->w = _w;
    }
    ~Quaternion() = default;

    Quaternion operator+(Quaternion q) const
    {
        return Quaternion {
            this->x + q.x,
            this->y + q.y,
            this->z + q.z,
            this->w + q.w
        };
    }

    Quaternion operator+(float _f) const
    {
        return Quaternion {
            this->x + _f,
            this->y + _f,
            this->z + _f,
            this->w + _f
        };
    }

    Quaternion& operator+=(Quaternion q)
    {
        *this = *this + q;
        return *this;
    }

    Quaternion& operator+=(float _f)
    {
        *this = *this + _f;
        return *this;
    }

    Quaternion operator-(Quaternion q) const
    {
        return Quaternion {
            this->x - q.x,
            this->y - q.y,
            this->z - q.z,
            this->w - q.w
        };
    }

    Quaternion operator-(float _f) const
    {
        return Quaternion {
            this->x - _f,
            this->y - _f,
            this->z - _f,
            this->w - _f
        };
    }

    Quaternion& operator-=(Quaternion q)
    {
        *this = *this - q;
        return *this;
    }

    Quaternion& operator-=(float _f) {
        *this = *this - _f;
        return *this;
    }


    Quaternion operator*(Quaternion q) const
    {
        return Quaternion {
            this->x * q.x,
            this->y * q.y,
            this->z * q.z,
            this->w * q.w
        };
    }

    Quaternion operator*(float _f) const
    {
        return Quaternion {
            this->x * _f,
            this->y * _f,
            this->z * _f,
            this->w * _f
        };
    }

    Quaternion& operator*=(Quaternion q)
    {
        *this = *this * q;
        return *this;
    }

    Quaternion& operator*=(float _f)
    {
        *this = *this * _f;
        return *this;
    }

    Quaternion operator/(Quaternion q) const
    {
        return Quaternion {
            this->x / q.x,
            this->y / q.y,
            this->z / q.z,
            this->w / q.w
        };
    }

    Quaternion operator/(float _f) const
    {
        return Quaternion {
            this->x / _f,
            this->y / _f,
            this->z / _f,
            this->w / _f
        };
    }

    Quaternion& operator/=(Quaternion q)
    {
        *this = *this / q;
        return *this;
    }

    Quaternion& operator/=(float _f)
    {
        *this = *this / _f;
        return *this;
    }

    bool operator==(Quaternion q) const
    {
        return this->x == q.x && this->y == q.y && this->z == q.z && this->w == q.w;
    }

    bool operator!=(Quaternion q) const
    {
        return !(*this == q);
    }

    bool IsEmpty() const
    {
        return !this->x && !this->y && !this->z && !this->w;
    }

    float SqrMagnitude() const
    {
        return (this->x * this->x) + (this->y * this->y) + (this->z * this->z) + (this->w * this->w);
    }

    float Magnitude() const
    {
        return sqrtf(this->SqrMagnitude());
    }

    static inline Quaternion Identity() { return Quaternion(0, 0, 0, 1); }
};
