#pragma once

struct Color
{
    float r, g, b, a;

    Color() = default;
    ~Color() = default;

    Color(float _r, float _g, float _b, float _a = 1.f)
    {
        this->r = _r;
        this->g = _g;
        this->b = _b;
        this->a = _a;
    }
};
