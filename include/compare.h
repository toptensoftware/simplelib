#pragma once

namespace SimpleLib
{

	// Default comparator
	struct SDefaultCompare
	{
		template <typename T>
		static int Compare(const T& a, const T& b)
		{
			return a > b ? 1 : a < b ? -1 : 0;
		}

		template <typename T>
		static bool AreEqual(const T& a, const T& b)
		{
			return a == b;
		}
	};

}

