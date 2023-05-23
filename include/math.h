#ifndef __simplelib_math_h__
#define __simplelib_math_h__

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

		inline const static double PI  = 3.141592653589793238463;
		inline const static float  PI_F = 3.14159265358979f;

		static double DegreesToRadian(double degrees)
		{
			return degrees * (PI / 180);
		}

	};

} // namespace

#endif  // __simplelib_math_h__

