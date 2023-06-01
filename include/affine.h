#pragma once

#include "vector.h"
#include "matrix4.h"

namespace SimpleLib
{
    template <typename T>
    class Affine
    {
    public:
        Affine()
        {
            this->M11 = 1;
            this->M12 = 0;
            this->M21 = 0;
            this->M22 = 1;
            this->M31 = 0;
            this->M32 = 0;
        }

        Affine(T M11, T M12, T M21, T M22, T M31, T M32)
        {
            this->M11 = M11;
            this->M12 = M12;
            this->M21 = M21;
            this->M22 = M22;
            this->M31 = M31;
            this->M32 = M32;
        }

        Affine<T> operator *(const Affine<T>& other) const
        {
            return Affine<T>(
                M11 * other.M11 + M12 * other.M21,
                    M11 * other.M12 + M12 * other.M22,
                M21 * other.M11 + M22 * other.M21,
                    M21 * other.M12 + M22 * other.M22,
                M31 * other.M11 + M32 * other.M21 +  other.M31,
                    M31 * other.M12 + M32 * other.M22 +  other.M32
                );
        }

        Affine<T> Translate(T dx, T dy) const
        {
            return *this * MakeTranslate(dx, dy);
        }

        Affine<T> Translate(const Vector<T>& offset) const
        {
            return *this * MakeTranslate(offset);
        }

        Affine<T> Rotate(double angle) const
        {
            return *this * MakeRotate(angle);
        }

        Affine<T> RotateAround(T x, T y, double angle) const
        {
            return *this * MakeRotateAround(x, y, angle);
        }

        Affine<T> RotateAround(const Vector<T>& pos, double angle) const
        {
            return *this * MakeRotateAround(pos, angle);
        }

        Affine<T> Scale(T scale) const
        {
            return *this * MakeScale(scale);
        }

        Affine<T> Scale(T scaleX, T scaleY) const
        {
            return *this * MakeScale(scaleX, scaleY);
        }

        Affine<T> Skew(T skew) const
        {
            return *this * MakeSkew(skew);
        }

        Affine<T> Inverse() const
        {
            T det = (M11 * M22) - (M12 * M21);
            T scale = 1.0 / det;

            return Affine<T>(
                scale * (M22),
                scale * (- M12),
                scale * (- M21),
                scale * (M11),
                scale * (M21 * M32 - M22 * M31),
                scale * (M12 * M31 - M11 * M32)
            );
        }

        T Determinant() const
        {
            return (M11 * M22) - (M12 * M21);
        }

        Matrix4<T> ToMatrix4() const
        {
            return Matrix4<T>(
                M11, M12, 0, 0,
                M21, M22, 0, 0,
                0, 0, 1, 0,
                M31, M32, 0, 1
            );
        }


        static Affine<T> MakeTranslate(T dx, T dy)
        {
            return Affine<T>(
                    1, 0,
                    0, 1,
                    dx, dy
                    );
        }

        static Affine<T> MakeTranslate(const Vector<T>& offset)
        {
            return Affine<T>(
                    1, 0,
                    0, 1,
                    offset.X, offset.Y
                    );
        }

        static Affine<T> MakeRotate(double angle)
        {
            double cosa = cos(angle);
            double sina = sin(angle);

            return Affine<T>(
                cosa, sina,
                -sina, cosa,
                0, 0
                );
        }

        static Affine<T> MakeScale(T scale)
        {
            return Affine<T>(
                scale, 0,
                0, scale,
                0, 0
                );
        }

        static Affine<T> MakeScale(T scaleX, T scaleY)
        {
            return Affine<T>(
                scaleX, 0,
                0, scaleY,
                0, 0
                );
        }

        static Affine<T> MakeSkew(T skew)
        {
            return Affine<T>(
                    1, 0,
                    skew, 1,
                    0, 0
                    );
        }

        static Affine<T> MakeRotateAround(T x, T y, double angle)
        {
            return MakeTranslate(-x, -y).Rotate(angle).Translate(x, y);
        }

        static Affine<T> MakeRotateAround(const Vector<T>& pos, double angle)
        {
            return MakeTranslate(-pos).Rotate(angle).Translate(pos);
        }

        T M11, M12, M21, M22, M31, M32;
    };

} // namespace
