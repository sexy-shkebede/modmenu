#pragma once

#include "Vector2.hpp"

namespace usdk
{
    struct SRect 
    {
    public:

        union
        {
            struct
            {
                float x, y, w, h;
            };
            Vector2 min;
            Vector2 max;
        };

        SRect() = default;
        ~SRect() = default;

        SRect(float _x, float _y, float _w, float _h)
        {
            this->x = _x;
            this->y = _y;
            this->w = _w;
            this->h = _h;
        }

        SRect(Vector2 _xy, Vector2 _wh)
        {
            this->x = _xy.x;
            this->y = _xy.y;
            this->w = _wh.x;
            this->h = _wh.y;
        }
    };
}
