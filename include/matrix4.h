#pragma once

namespace SimpleLib
{
    template <typename T>
    class Matrix4
    {
    public:
        Matrix4()
        {
            M11 = 1; M12 = 0; M13 = 0; M14 = 0;
            M21 = 0; M22 = 1; M23 = 0; M24 = 0;
            M31 = 0; M32 = 0; M33 = 1; M34 = 0;
            M41 = 0; M42 = 0; M43 = 0; M44 = 1;
        }
        Matrix4(
            T m11, T m12, T m13, T m14,
            T m21, T m22, T m23, T m24,
            T m31, T m32, T m33, T m34,
            T m41, T m42, T m43, T m44
            )
        {
            M11 = m11; M12 = m12; M13 = m13; M14 = m14;
            M21 = m21; M22 = m22; M23 = m23; M24 = m24;
            M31 = m31; M32 = m32; M33 = m33; M34 = m34;
            M41 = m41; M42 = m42; M43 = m43; M44 = m44;
        }

        Matrix4(const Matrix4<T>& other)
        {
            memcpy(this, &other, sizeof(Matrix4<T>));
        }

        Matrix4<T> Inverse() const
        {
            Matrix4<T> inv(
                M22 * M33 * M44 -
                M22 * M34 * M43 -
                M32 * M23 * M44 +
                M32 * M24 * M43 +
                M42 * M23 * M34 -
                M42 * M24 * M33,

                -M12 * M33 * M44 +
                M12 * M34 * M43 +
                M32 * M13 * M44 -
                M32 * M14 * M43 -
                M42 * M13 * M34 +
                M42 * M14 * M33,

                M12 * M23 * M44 -
                M12 * M24 * M43 -
                M22 * M13 * M44 +
                M22 * M14 * M43 +
                M42 * M13 * M24 -
                M42 * M14 * M23,

                -M12 * M23 * M34 +
                M12 * M24 * M33 +
                M22 * M13 * M34 -
                M22 * M14 * M33 -
                M32 * M13 * M24 +
                M32 * M14 * M23,

                -M21 * M33 * M44 +
                M21 * M34 * M43 +
                M31 * M23 * M44 -
                M31 * M24 * M43 -
                M41 * M23 * M34 +
                M41 * M24 * M33,


                M11 * M33 * M44 -
                M11 * M34 * M43 -
                M31 * M13 * M44 +
                M31 * M14 * M43 +
                M41 * M13 * M34 -
                M41 * M14 * M33,

                -M11 * M23 * M44 +
                M11 * M24 * M43 +
                M21 * M13 * M44 -
                M21 * M14 * M43 -
                M41 * M13 * M24 +
                M41 * M14 * M23,

                M11 * M23 * M34 -
                M11 * M24 * M33 -
                M21 * M13 * M34 +
                M21 * M14 * M33 +
                M31 * M13 * M24 -
                M31 * M14 * M23,

                M21 * M32 * M44 -
                M21 * M34 * M42 -
                M31 * M22 * M44 +
                M31 * M24 * M42 +
                M41 * M22 * M34 -
                M41 * M24 * M32,

                -M11 * M32 * M44 +
                M11 * M34 * M42 +
                M31 * M12 * M44 -
                M31 * M14 * M42 -
                M41 * M12 * M34 +
                M41 * M14 * M32,

                M11 * M22 * M44 -
                M11 * M24 * M42 -
                M21 * M12 * M44 +
                M21 * M14 * M42 +
                M41 * M12 * M24 -
                M41 * M14 * M22,

                -M11 * M22 * M34 +
                M11 * M24 * M32 +
                M21 * M12 * M34 -
                M21 * M14 * M32 -
                M31 * M12 * M24 +
                M31 * M14 * M22,

                -M21 * M32 * M43 +
                M21 * M33 * M42 +
                M31 * M22 * M43 -
                M31 * M23 * M42 -
                M41 * M22 * M33 +
                M41 * M23 * M32,

                M11 * M32 * M43 -
                M11 * M33 * M42 -
                M31 * M12 * M43 +
                M31 * M13 * M42 +
                M41 * M12 * M33 -
                M41 * M13 * M32,


                -M11 * M22 * M43 +
                M11 * M23 * M42 +
                M21 * M12 * M43 -
                M21 * M13 * M42 -
                M41 * M12 * M23 +
                M41 * M13 * M22,


                M11 * M22 * M33 -
                M11 * M23 * M32 -
                M21 * M12 * M33 +
                M21 * M13 * M32 +
                M31 * M12 * M23 -
                M31 * M13 * M22
                );

            T det = M11 * inv.M11 + M12 * inv.M21 + M13 * inv.M31 + M14 * inv.M41;
            det = 1.0 / det;

            inv.M11 *= det;
            inv.M12 *= det;
            inv.M13 *= det;
            inv.M14 *= det;
            inv.M21 *= det;
            inv.M22 *= det;
            inv.M23 *= det;
            inv.M24 *= det;
            inv.M31 *= det;
            inv.M32 *= det;
            inv.M33 *= det;
            inv.M34 *= det;
            inv.M41 *= det;
            inv.M42 *= det;
            inv.M43 *= det;
            inv.M44 *= det;
            return inv;
        }

        static Matrix4<T> MakeOrthoProjection(T left, T top, T right, T bottom, T near, T far)
        {
            return Matrix4<T>(

                2.0 / (right - left),
                0.0,
                0.0,
                0.0,

                0.0,
                2.0 / (top - bottom),
                0.0,
                0.0,

                0,
                0,
                -2.0 / (far - near),
                0.0,

                -(right + left) / (right - left),
                -(top + bottom) / (top - bottom),
                -(far + near) / (far - near),
                1.0
            );
        }

        Matrix4<T> Translate(T dx, T dy, T dz) const
        {
            return *this * MakeTranslate(dx, dy, dz);
        }

        Matrix4 RotateX(double a) const
        {
            return *this * MakeRotateX(a);
        }

        Matrix4 RotateY(double a) const
        {
            return *this * MakeRotateY(a);
        }

        Matrix4 RotateZ(double a) const
        {
            return *this * MakeRotateZ(a);
        }

        Matrix4<T> Scale(T scaleX, T scaleY, T scaleZ) const
        {
            return *this * MakeScale(scaleX, scaleY, scaleZ);
        }

        Matrix4<T> Scale(T scale) const
        {
            return *this * MakeScale(scale);
        }

        Matrix4<T> Transpose() const
        {
            return Matrix4<T>(
                M11, M21, M31, M41,
                M12, M22, M32, M42,
                M13, M23, M33, M43,
                M14, M24, M34, M44
            );
        }

        static Matrix4<T> MakeTranslate(T dx, T dy, T dz)
        {
            return Matrix4<T>(
                1, 0, 0, 0,
                0, 1, 0, 0,
                0, 0, 1, 0,
                dx, dy, dz, 1
                );
        }

        static Matrix4<T> MakeRotateX(double a)
        {
            double cosa = cos(a);
            double sina = sin(a);
            return Matrix4<T>(
                1, 0, 0, 0,
                0, cosa, sina, 0,
                0, -sina, cosa, 0,
                0, 0, 0, 1
                );
        }

        static Matrix4<T> MakeRotateY(double a)
        {
            double cosa = cos(a);
            double sina = sin(a);
            return Matrix4<T>(
                cosa, 0, -sina, 0,
                0, 1, 0, 0,
                sina, 0, cosa, 0,
                0, 0, 0, 1
            );
        }

        static Matrix4<T> MakeRotateZ(double a)
        {
            double cosa = cos(a);
            double sina = sin(a);
            return Matrix4<T>(
                cosa, sina, 0, 0,
                -sina, cosa, 0, 0,
                0, 0, 1, 0,
                0, 0, 0, 1
            );
        }

        static Matrix4<T> MakeScale(T scaleX, T scaleY, T scaleZ)
        {
            return Matrix4<T>(
                scaleX, 0, 0, 0,
                0, scaleY, 0, 0,
                0, 0, scaleZ, 0,
                0, 0, 0, 1
                );
        }

        static Matrix4<T> MakeScale(T scale)
        {
            return Matrix4<T>(
                scale, 0, 0, 0,
                0, scale, 0, 0,
                0, 0, scale, 0,
                0, 0, 0, 1
                );
        }

        Matrix4<T> operator*(const Matrix4<T>& other) const
        {
            return Matrix4<T>(
                M11 * other.M11 + M12 * other.M21 + M13 * other.M31 + M14 * other.M41,
                    M11 * other.M12 + M12 * other.M22 + M13 * other.M32 + M14 * other.M42,
                    M11 * other.M13 + M12 * other.M23 + M13 * other.M33 + M14 * other.M43,
                    M11 * other.M14 + M12 * other.M24 + M13 * other.M34 + M14 * other.M44,
                M21 * other.M11 + M22 * other.M21 + M23 * other.M31 + M24 * other.M41,
                    M21 * other.M12 + M22 * other.M22 + M23 * other.M32 + M24 * other.M42,
                    M21 * other.M13 + M22 * other.M23 + M23 * other.M33 + M24 * other.M43,
                    M21 * other.M14 + M22 * other.M24 + M23 * other.M34 + M24 * other.M44,
                M31 * other.M11 + M32 * other.M21 + M33 * other.M31 + M34 * other.M41,
                    M31 * other.M12 + M32 * other.M22 + M33 * other.M32 + M34 * other.M42,
                    M31 * other.M13 + M32 * other.M23 + M33 * other.M33 + M34 * other.M43,
                    M31 * other.M14 + M32 * other.M24 + M33 * other.M34 + M34 * other.M44,
                M41 * other.M11 + M42 * other.M21 + M43 * other.M31 + M44 * other.M41,
                    M41 * other.M12 + M42 * other.M22 + M43 * other.M32 + M44 * other.M42,
                    M41 * other.M13 + M42 * other.M23 + M43 * other.M33 + M44 * other.M43,
                    M41 * other.M14 + M42 * other.M24 + M43 * other.M34 + M44 * other.M44
                );
        }


        T M11, M12, M13, M14;
        T M21, M22, M23, M24;
        T M31, M32, M33, M34;
        T M41, M42, M43, M44;

    };

}