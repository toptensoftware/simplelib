#pragma once

namespace SimpleLib
{
    class ColorF
    {
    public:
        ColorF(float red, float green, float blue, float alpha = 1)
        {
            r = red;
            g = green;
            b = blue;
            a = alpha;
        }
        ColorF(uint32_t rgba)
        {
            r = ((rgba >> 24) & 0xFF) / 255.0f;
            g = ((rgba >> 16) & 0xFF) / 255.0f;
            b = ((rgba >> 8) & 0xFF) / 255.0f;
            a = ((rgba >> 0) & 0xFF) / 255.0f;
        }
        float r;
        float g;
        float b;
        float a;
    };
}
