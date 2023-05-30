#ifndef __simplelib_rectangle_h__
#define __simplelib_rectangle_h__

#include "vector.h"
#include "math.h"

namespace SimpleLib
{
	template <typename T>
	class Rectangle
	{
	public:
		Rectangle()
		{
			Left = 0;
			Top = 0;
			Width = 0;
			Height = 0;
		}

		Rectangle(T left, T top, T width, T height)
		{
			Left = left;
			Top = top;
			Width = width;
			Height = height;
		}

		Rectangle<T> Intersection(const Rectangle<T>& other) const
		{
			if (!this->IsNormal())
				return other;
			if (!other.IsNormal())
				return *this;

			T left = Math::Max(Left, other.Left);
			T top = Math::Max(Top, other.Top);
			return Rectangle(
				left,
				top,
				Math::Min(Right(), other.Right()) - left,
				Math::Min(Bottom(), other.Bottom()) - top
			);
		}

		Rectangle Union(const Rectangle& other) const
		{
			if (!this->IsNormal())
				return other;
			if (!other.IsNormal())
				return *this;

			T left = Math::Min(Left, other.Left);
			T top = Math::Min(Top, other.Top);
			return Rectangle(
				left,
				top,
				Math::Max(Right(), other.Right()) - left,
				Math::Max(Bottom(), other.Bottom()) - top
			);
		}

		bool IsNormal() const
		{
			return Width > 0 && Height > 0;
		}

		T Right() const
		{
			return Left + Width;
		}

		void setRight(T value)
		{
			Width = value - Left;
		}

		T Bottom() const
		{
			return Top + Height;
		}

		void setBottom(T value)
		{
			Height = value - Top;
		}

		Vector<T> Center() const
		{
			return Vector<T>((Left + Right) / 2, (Top + Bottom) / 2);
		}

		Vector<T> TopLeft() const
		{
			return Vector<T>(Left, Top);
		}

		Vector<T> TopRight() const
		{
			return Vector<T>(Right(), Top);
		}

		Vector<T> BottomLeft() const
		{
			return Vector<T>(Left, Bottom());
		}

		Vector<T> BottomRight() const
		{
			return Vector<T>(Right(), Bottom());
		}

		T CenterX() const
		{
			return (Left + Right) / 2;
		}

		T CenterY() const
		{
			return (Top + Bottom) / 2;
		}

		T Area() const
		{
			return Width * Height;
		}

		Rectangle Offset(T dx, T dy) const
		{
			return Rectangle(Left + dx, Top + dy, Width, Height);
		}

		Rectangle Inflate(T dx, T dy) const
		{
			return Rectangle(Left - dx, Top - dy, Width + dx + dx, Height + dy + dy);
		}

		Rectangle Inset(T left, T top, T right, T bottom) const
		{
			return Rectangle(Left + left, Top + top, Width - left - right, Height - top - bottom);
		}

		Rectangle Outset(T left, T top, T right, T bottom) const
		{
			return Rectangle(Left - left, Top - top, Width + left + right, Height + top + bottom);
		}

		bool Contains(T x, T y) const
		{
			return x >= Left && x <= Right() && y >= Top && y <= Bottom();
		}

		bool Contains(const Vector<T>& point)
		{
			return Contains(point.X, point.Y);
		}

		bool Contains(const Rectangle<T> other)
		{
			return other.Left >= Left && other.Right() <= Right() && other.Top >= Top && other.Bottom() <= Bottom();
		}

		static Rectangle FromCoords(T x1, T y1, T x2, T y2)
		{
			return Rectangle(x1, y1, x2 - x1, y2 - y1);
		}

		bool operator==(const Rectangle<T>& other)
		{
			return Left == other.Left && Top == other.Top && Width == other.Width && Height == other.Height;
		}

		bool operator!=(const Rectangle<T>& other)
		{
			return !(*this == other);
		}


		T Left;
		T Top;
		T Width;
		T Height;
	};

	typedef Rectangle<int> RectangleI;
	typedef Rectangle<double> RectangleD;
	typedef Rectangle<float> RectangleF;

} // namespace

#endif  // __simplelib_rectangle_h__

