#pragma once

namespace SimpleLib
{
	template <typename T>
	class Vector
	{
	public:
		Vector()
		{
			X = 0;
			Y = 0;
		}

		Vector(T x, T y)
		{
			X = x;
			Y = y;
		}

		Vector<T> operator-() const
		{
			return Vector(-X, -Y);
		}
		
		Vector<T> operator+(const Vector<T>& other) const
		{
			return Vector(X + other.X, Y + other.Y);
		}

		Vector<T> operator-(const Vector<T>& other) const
		{
			return Vector(X - other.X, Y - other.Y);
		}

		Vector<T> operator*(T scalar) const
		{
			return Vector(X * scalar, Y * scalar);
		}

		Vector<T> operator/(T scalar) const
		{
			return Vector(X / scalar, Y * scalar);
		}

		T Magnitude() const
		{
			return (T)sqrt(X * X + Y * Y);
		}

		static T Distance(const Vector<T>& a, const Vector<T>& b)
		{
			return (b-a).Magnitude();
		}

        /// Check if this point is inside a triangle
        bool IsInsideTriangle(const Vector<T>& A, const Vector<T>& B, const Vector<T>& C) const
        {
            T ax = C.X - B.X;
            T ay = C.Y - B.Y;
            T bpx = X - B.X;
            T bpy = Y - B.Y;
            T aCROSSbp = ax * bpy - ay * bpx;
            if (aCROSSbp < 0)
                return false;

            T bx = A.X - C.X;
            T by = A.Y - C.Y;
            T cpx = X - C.X;
            T cpy = Y - C.Y;
            T bCROSScp = bx * cpy - by * cpx;
            if (bCROSScp < 0)
                return false;

            T cx = B.X - A.X;
            T cy = B.Y - A.Y;
            T apx = X - A.X;
            T apy = Y - A.Y;
            T cCROSSap = cx * apy - cy * apx;
            return cCROSSap >= 0;
        }

		Vector<T> Normal() const
		{
			return Vector<T>(Y, -X);
		}

		static T DotProduct(const Vector<T>& pointA, const Vector<T>& pointB, const Vector<T>& pointC)
        {
            Vector<T> AB = pointB - pointA;
            Vector<T> BC = pointC - pointB;
            return AB.X * BC.X + AB.Y * BC.Y;
        }

        static T CrossProduct(const Vector<T>& pointA, const Vector<T>& pointB, const Vector<T>& pointC)
        {
            Vector<T> AB = pointB - pointA;
            Vector<T> AC = pointC - pointA;
            return AB.X * AC.Y - AB.Y * AC.X;
        }

		bool operator==(const Vector<T>& other)
		{
			return X == other.X && Y == other.Y;
		}

		bool operator!=(const Vector<T>& other)
		{
			return !(*this == other);
		}

		Vector Rotate(double angle)
		{
			double sina = sin(angle);
			double cosa = cos(angle);
			return Vector<T>((T)(X * cosa - Y * sina), (T)(X * sina + Y * cosa));
		}

		T X;
		T Y;
	};

	typedef Vector<int> VectorI;
	typedef Vector<double> VectorD;
	typedef Vector<float> VectorF;

}
