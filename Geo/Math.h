#pragma once

#include <math.h>

namespace SimpleLib
{
	class Math
	{
	public:
		// Minimum
		template <typename T>
		static T Min(const T& a, const T& b) { return a < b ? a : b; }

		// Maximum
		template <typename T>
		static T Max(const T& a, const T& b) { return a > b ? a : b; }
		
		template <typename T>
		static void Normalize(T& a, T& b)
		{
			if (a > b)
			{
				T temp = a;
				a = b;
				b = temp;
			}
		}

		template <typename T>
		static T Constrain(T val, T min, T max)
		{
			if (min < max)
			{
				// Normal range
				if (val < min)
					return min;
				if (val > max)
					return max;
			}
			else
			{
				// Inverse range
				if (val < max)
					return max;
				if (val > min)
					return min;
			}
			return val;
		}


		inline static constexpr double PI = 3.141592653589793238463;
		inline static constexpr double PI_F = 3.14159265358979f;

		static double DegreesToRadian(double degrees)
		{
			return degrees * (PI / 180);
		}

	};
}