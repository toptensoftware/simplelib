#pragma once

namespace SimpleLib
{
    class ColorF
    {
    public:
        ColorF()
        {
            r = 0;
            g = 0;
            b = 0;
            a = 0;   
        }
        // Constructor
        ColorF(float red, float green, float blue, float alpha = 1)
        {
            r = red;
            g = green;
            b = blue;
            a = alpha;
        }

        // Color components
        float r;
        float g;
        float b;
        float a;

        uint32_t AsARGB() const
        {
            return 
                (((uint32_t)(a * 255) & 0xFF) << 24) | 
                (((uint32_t)(r * 255) & 0xFF) << 16) |
                (((uint32_t)(g * 255) & 0xFF) << 8) |
                (((uint32_t)(b * 255) & 0xFF) << 0);
        }

        uint32_t AsABGR() const
        {
            return 
                (((uint32_t)(a * 255) & 0xFF) << 24) | 
                (((uint32_t)(b * 255) & 0xFF) << 16) |
                (((uint32_t)(g * 255) & 0xFF) << 8) |
                (((uint32_t)(r * 255) & 0xFF) << 0);
        }

        uint32_t AsRGBA() const
        {
            return 
                (((uint32_t)(r * 255) & 0xFF) << 24) | 
                (((uint32_t)(g * 255) & 0xFF) << 16) |
                (((uint32_t)(b * 255) & 0xFF) << 8) |
                (((uint32_t)(a * 255) & 0xFF) << 0);
        }

        // Interoplate between two colors
        static ColorF Interpolate(const ColorF& A, const ColorF& B, double pos)
        {
            return ColorF(
                (float)(A.r + (B.r - A.r) * pos),
                (float)(A.g + (B.g - A.g) * pos),
                (float)(A.b + (B.b - A.b) * pos),
                (float)(A.a + (B.a - A.a) * pos)
            );
        }

        // Blend two colors
        // See: https://en.wikipedia.org/wiki/Alpha_compositing
        static ColorF Blend(const ColorF& back, const ColorF& fore)
        {
            if (fore.a == 1)
                return fore;

            if (fore.a == 0)
                return back;

            float oneMinusForeAlpha = 1 - fore.a;
            float a0 = fore.a + back.a * oneMinusForeAlpha;
            return ColorF(
                (fore.r * fore.a + back.r * back.a * oneMinusForeAlpha) / a0,
                (fore.g * fore.a + back.g * back.a * oneMinusForeAlpha) / a0,
                (fore.b * fore.a + back.b * back.a * oneMinusForeAlpha) / a0,
                a0
            );
        }

        // 0xAARRGGBB
        static ColorF FromARGB(uint32_t value)
        {
            return ColorF(
                ((value >> 16) & 0xFF) / 255.0f, 
                ((value >> 8) & 0xFF) / 255.0f, 
                ((value >> 0) & 0xFF) / 255.0f, 
                ((value >> 24) & 0xFF) / 255.0f
            );
        }

        // 0xAABBGGRR
        static ColorF FromABGR(uint32_t value)
        {
            return ColorF(
                ((value >> 0) & 0xFF) / 255.0f, 
                ((value >> 8) & 0xFF) / 255.0f, 
                ((value >> 16) & 0xFF) / 255.0f, 
                ((value >> 24) & 0xFF) / 255.0f
            );
        }

        // 0xRRGGBAA
        static ColorF FromRGBA(uint32_t value)
        {
            return ColorF(
                ((value >> 24) & 0xFF) / 255.0f,
                ((value >> 16) & 0xFF) / 255.0f, 
                ((value >> 8) & 0xFF) / 255.0f, 
                ((value >> 0) & 0xFF) / 255.0f
            );
        }

    };
}
