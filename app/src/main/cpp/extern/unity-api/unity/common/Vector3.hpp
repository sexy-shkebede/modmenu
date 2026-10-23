#pragma once

#include <math.h>

struct Vector3
{
public:
    float x;
    float y;
    float z;

    Vector3() = default;
    Vector3(float _x, float _y, float _z)
    {
        this->x = _x;
        this->y = _y;
        this->z = _z;
    }
    ~Vector3() = default;

    Vector3 operator-() const
    {
        return Vector3{
            -this->x,
            -this->y,
            -this->z
        };
    }
    
    Vector3 operator+(Vector3 vec) const
    {
        return Vector3 {
            this->x + vec.x,
            this->y + vec.y,
            this->z + vec.z
        };
    }

    Vector3 operator+(float _f) const
    {
        return Vector3 {
            this->x + _f,
            this->y + _f,
            this->z + _f
        };
    }

    Vector3& operator+=(Vector3 vec)
    {
        *this = *this + vec;
        return *this;
    }

    Vector3& operator+=(float _f)
    {
        *this = *this + _f;
        return *this;
    }

    Vector3 operator-(Vector3 vec) const
    {
        return Vector3 {
            this->x - vec.x,
            this->y - vec.y,
            this->z - vec.z
        };
    }

    Vector3 operator-(float _f) const
    {
        return Vector3 {
            this->x - _f,
            this->y - _f,
            this->z - _f
        };
    }

    Vector3& operator-=(Vector3 vec)
    {
        *this = *this - vec;
        return *this;
    }

    Vector3& operator-=(float _f)
    {
        *this = *this - _f;
        return *this;
    }

    Vector3 operator*(Vector3 vec) const
    {
        return Vector3 {
            this->x * vec.x,
            this->y * vec.y,
            this->z * vec.z
        };
    }

    Vector3 operator*(float _f) const
    {
        return Vector3 {
            this->x * _f,
            this->y * _f,
            this->z * _f
        };
    }

    Vector3& operator*=(Vector3 vec)
    {
        *this = *this * vec;
        return *this;
    }

    Vector3& operator*=(float _f)
    {
        *this = *this * _f;
        return *this;
    }

    Vector3 operator/(Vector3 vec) const
    {
        return Vector3 {
            this->x / vec.x,
            this->y / vec.y,
            this->z / vec.z
        };
    }

    Vector3 operator/(float _f) const
    {
        return Vector3 {
            this->x / _f,
            this->y / _f,
            this->z / _f
        };
    }

    Vector3& operator/=(Vector3 vec)
    {
        *this = *this / vec;
        return *this;
    }

    Vector3& operator/=(float _f)
    {
        *this = *this / _f;
        return *this;
    }

    bool operator==(Vector3 vec) const
    {
        return this->x == vec.x && this->y == vec.y && this->z == vec.z;
    }

    bool operator!=(Vector3 vec) const
    {
        return !(*this == vec);
    }

    bool isEmpty() const
    {
        return !this->x && !this->y && !this->z;
    }

    float SqrMagnitude() const
    {
        return (this->x * this->x) + (this->y * this->y) + (this->z * this->z);
    }

    float Magnitude() const
    {
        return sqrtf(this->SqrMagnitude());
    }

    float DistanceTo(const Vector3& vec) const
    {
        return (*this - vec).Magnitude();
    }

    void Normalize()
    {
        const float fMagnitude = this->Magnitude();
        if(fMagnitude != 0.f)
            *this /= fMagnitude;
    }

    Vector3 Normalized()
    {
        Vector3 ret = *this;
        ret.Normalize();
        return ret;
    }

    float Dot(const Vector3& vec) const
    {
        return (this->x * vec.x) + (this->y * vec.y) + (this->z * vec.z);
    }
    
    static inline Vector3 Up()      { return Vector3(0, 1, 0);  }
    static inline Vector3 Down()    { return Vector3(0, -1, 0); }
    static inline Vector3 Left()    { return Vector3(-1, 0, 0); }
    static inline Vector3 Right()   { return Vector3(1, 0, 0);  }
    static inline Vector3 Forward() { return Vector3(0, 0, 1);  }
    static inline Vector3 Back()    { return Vector3(0, 0, -1); }
    static inline Vector3 Zero()    { return Vector3(0, 0, 0);  }
    static inline Vector3 One()     { return Vector3(1, 1, 1);  }
};
