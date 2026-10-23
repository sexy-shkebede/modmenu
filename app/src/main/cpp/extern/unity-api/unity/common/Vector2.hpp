#pragma once

#include <math.h>

struct Vector2
{
public:
    float x;
    float y;

    Vector2() = default;

    Vector2(const float& _x, const float& _y)
    {
        this->x = _x;
        this->y = _y;
    }

    ~Vector2() = default;
    
    Vector2 operator+(Vector2 vec) const
    {
        return Vector2 {
            this->x + vec.x,
            this->y + vec.y
        };
    }

    Vector2 operator+(float _f) const
    {
        return Vector2 {
            this->x + _f,
            this->y + _f
        };
    }

    Vector2& operator+=(Vector2 vec)
    {
        *this = *this + vec;
        return *this;
    }

    Vector2& operator+=(float _f)
    {
        *this = *this + _f;
        return *this;
    }

    Vector2 operator-(Vector2 vec) const
    {
        return Vector2 {
            this->x - vec.x,
            this->y - vec.y
        };
    }

    Vector2 operator-(float _f) const
    {
        return Vector2 {
            this->x - _f,
            this->y - _f
        };
    }

    Vector2& operator-=(Vector2 vec)
    {
        *this = *this - vec;
        return *this;
    }

    Vector2& operator-=(float _f)
    {
        *this = *this - _f;
        return *this;
    }

    Vector2 operator*(Vector2 vec) const
    {
        return Vector2 {
            this->x * vec.x,
            this->y * vec.y
        };
    }

    Vector2 operator*(float _f) const
    {
        return Vector2 {
            this->x * _f,
            this->y * _f
        };
    }

    Vector2& operator*=(Vector2 vec)
    {
        *this = *this * vec;
        return *this;
    }

    Vector2& operator*=(float _f)
    {
        *this = *this * _f;
        return *this;
    }

    Vector2 operator/(Vector2 vec) const
    {
        return Vector2 {
            this->x / vec.x,
            this->y / vec.y
        };
    }

    Vector2 operator/(float _f) const
    {
        return Vector2 {
            this->x / _f,
            this->y / _f
        };
    }

    Vector2& operator/=(Vector2 vec)
    {
        *this = *this / vec;
        return *this;
    }

    Vector2& operator/=(float _f)
    {
        *this = *this / _f;
        return *this;
    }

    bool operator==(Vector2 vec) const
    {
        return this->x == vec.x && this->y == vec.y;
    }

    bool operator!=(Vector2 vec) const
    {
        return !(*this == vec);
    }

    bool IsEmpty() const
    {
        return !this->x && !this->y;
    }

    float SqrMagnitude() const
    {
        return (this->x * this->x) + (this->y * this->y);
    }

    float Magnitude() const
    {
        return sqrtf(this->SqrMagnitude());
    }

    float DistanceTo(const Vector2& vec) const
    {
        return (*this - vec).Magnitude();
    }

    void Normalize()
    {
        const float fMagnitude = this->Magnitude();
        if(fMagnitude != 0.f)
            *this /= fMagnitude;
    }

    Vector2 Normalized()
    {
        Vector2 ret = *this;
        ret.Normalize();
        return ret;
    }
};
