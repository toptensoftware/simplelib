#ifndef __simplelib_caseconversion_h__
#define __simplelib_caseconversion_h__

#include "vector.h"

namespace SimpleLib
{
	struct SCaseConversion
	{
		static char32_t ToUpper(char32_t t)
		{
			// ASCII/ANSI fast path
			if (t < 0x7f)
			{
				if (t >= 'a' && t <= 'z')
					return t + ('A' - 'a');
				else
					return t;
			}

			int count;
			auto map = GetToUpperMap(count);
			return Map(t, map, count);
		}

		static char32_t ToLower(char32_t t)
		{
			// ASCII/ANSI fast path
			if (t < 0x7f)
			{
				if (t >= 'A' && t <= 'Z')
					return t + ('a' - 'A');
				else
					return t;
			}			

			int count;
			auto map = GetToLowerMap(count);
			return Map(t, map, count);
		}

		struct CaseMapEntry
		{
			char16_t start;
			char16_t count;
			short offset;
		};

    	static int __cdecl findMapEntry(void const* a, void const* b)
		{
			char32_t ch = *(char32_t*)a;
			CaseMapEntry* e = (CaseMapEntry*)b;
			if (ch < e->start)
				return -1;
			if (ch >= e->start + e->count)
				return 1;
			return 0;
		}

		static char32_t Map(char32_t t, const CaseMapEntry* map, int mapCount)
		{
			CaseMapEntry* pEntry = (CaseMapEntry*)bsearch(&t, map, mapCount, sizeof(CaseMapEntry), findMapEntry);
			if (pEntry)
				return t + pEntry->offset;
			else
				return t;
		}

		static int __cdecl compareMapEntries(const void* a, const void* b)
		{
			return ((const CaseMapEntry*)a)->start - ((const CaseMapEntry*)b)->start;
		}

		static const CaseMapEntry* GetToLowerMap(int& count)
		{
			static CaseMapEntry* map = nullptr;
			static int mapCount = 0;
			if (map == nullptr)
			{
				// Create reverse map
				const CaseMapEntry* tuMap = GetToUpperMap(count);
				CVector<CaseMapEntry> tlMap;
				const CaseMapEntry* pForward = tuMap;
				while (pForward < tuMap + count)
				{
					// Ignrore u0131 LATIN SMALL LETTER DOTLESS I -> u0049 LATIN CAPITAL LETTER I
					if (pForward->start != 0x131)
					{
						CaseMapEntry rev;
						rev.start = pForward->start + pForward->offset;
						rev.count = pForward->count;
						rev.offset = -pForward->offset;
						tlMap.Add(rev);
					}
					pForward++;
				}

				// Sort it
				qsort(tlMap.GetBuffer(), tlMap.GetCount(), sizeof(CaseMapEntry), compareMapEntries);

				// Detach and never release
				map = tlMap.Detach(&mapCount);
			}

			count = mapCount;
			return map;
		}

		static const CaseMapEntry* GetToUpperMap(int& count)
		{
			// See tools directory for script to generate this
			static CaseMapEntry map[] = {
0x0061,26,-32,	0x00e0,23,-32,	0x00f8,7,-32,	0x00ff,1,121,	0x0101,1,-1,	0x0103,1,-1,	0x0105,1,-1,	0x0107,1,-1,	
0x0109,1,-1,	0x010b,1,-1,	0x010d,1,-1,	0x010f,1,-1,	0x0111,1,-1,	0x0113,1,-1,	0x0115,1,-1,	0x0117,1,-1,	
0x0119,1,-1,	0x011b,1,-1,	0x011d,1,-1,	0x011f,1,-1,	0x0121,1,-1,	0x0123,1,-1,	0x0125,1,-1,	0x0127,1,-1,	
0x0129,1,-1,	0x012b,1,-1,	0x012d,1,-1,	0x012f,1,-1,	0x0131,1,-232,	0x0133,1,-1,	0x0135,1,-1,	0x0137,1,-1,	
0x013a,1,-1,	0x013c,1,-1,	0x013e,1,-1,	0x0140,1,-1,	0x0142,1,-1,	0x0144,1,-1,	0x0146,1,-1,	0x0148,1,-1,	
0x014b,1,-1,	0x014d,1,-1,	0x014f,1,-1,	0x0151,1,-1,	0x0153,1,-1,	0x0155,1,-1,	0x0157,1,-1,	0x0159,1,-1,	
0x015b,1,-1,	0x015d,1,-1,	0x015f,1,-1,	0x0161,1,-1,	0x0163,1,-1,	0x0165,1,-1,	0x0167,1,-1,	0x0169,1,-1,	
0x016b,1,-1,	0x016d,1,-1,	0x016f,1,-1,	0x0171,1,-1,	0x0173,1,-1,	0x0175,1,-1,	0x0177,1,-1,	0x017a,1,-1,	
0x017c,1,-1,	0x017e,1,-1,	0x0183,1,-1,	0x0185,1,-1,	0x0188,1,-1,	0x018c,1,-1,	0x0192,1,-1,	0x0199,1,-1,	
0x01a1,1,-1,	0x01a3,1,-1,	0x01a5,1,-1,	0x01a8,1,-1,	0x01ad,1,-1,	0x01b0,1,-1,	0x01b4,1,-1,	0x01b6,1,-1,	
0x01b9,1,-1,	0x01bd,1,-1,	0x01c6,1,-2,	0x01c9,1,-2,	0x01cc,1,-2,	0x01ce,1,-1,	0x01d0,1,-1,	0x01d2,1,-1,	
0x01d4,1,-1,	0x01d6,1,-1,	0x01d8,1,-1,	0x01da,1,-1,	0x01dc,1,-1,	0x01df,1,-1,	0x01e1,1,-1,	0x01e3,1,-1,	
0x01e5,1,-1,	0x01e7,1,-1,	0x01e9,1,-1,	0x01eb,1,-1,	0x01ed,1,-1,	0x01ef,1,-1,	0x01f3,1,-2,	0x01f5,1,-1,	
0x01fb,1,-1,	0x01fd,1,-1,	0x01ff,1,-1,	0x0201,1,-1,	0x0203,1,-1,	0x0205,1,-1,	0x0207,1,-1,	0x0209,1,-1,	
0x020b,1,-1,	0x020d,1,-1,	0x020f,1,-1,	0x0211,1,-1,	0x0213,1,-1,	0x0215,1,-1,	0x0217,1,-1,	0x0253,1,-210,	
0x0254,1,-206,	0x0257,1,-205,	0x0258,2,-202,	0x025b,1,-203,	0x0260,1,-205,	0x0263,1,-207,	0x0268,1,-209,	0x0269,1,-211,	
0x026f,1,-211,	0x0272,1,-213,	0x0275,1,-214,	0x0283,1,-218,	0x0288,1,-218,	0x028a,2,-217,	0x0292,1,-219,	0x03ac,1,-38,	
0x03ad,3,-37,	0x03b1,17,-32,	0x03c3,9,-32,	0x03cc,1,-64,	0x03cd,2,-63,	0x03e3,1,-1,	0x03e5,1,-1,	0x03e7,1,-1,	
0x03e9,1,-1,	0x03eb,1,-1,	0x03ed,1,-1,	0x03ef,1,-1,	0x0430,32,-32,	0x0451,12,-80,	0x045e,2,-80,	0x0461,1,-1,	
0x0463,1,-1,	0x0465,1,-1,	0x0467,1,-1,	0x0469,1,-1,	0x046b,1,-1,	0x046d,1,-1,	0x046f,1,-1,	0x0471,1,-1,	
0x0473,1,-1,	0x0475,1,-1,	0x0477,1,-1,	0x0479,1,-1,	0x047b,1,-1,	0x047d,1,-1,	0x047f,1,-1,	0x0481,1,-1,	
0x0491,1,-1,	0x0493,1,-1,	0x0495,1,-1,	0x0497,1,-1,	0x0499,1,-1,	0x049b,1,-1,	0x049d,1,-1,	0x049f,1,-1,	
0x04a1,1,-1,	0x04a3,1,-1,	0x04a5,1,-1,	0x04a7,1,-1,	0x04a9,1,-1,	0x04ab,1,-1,	0x04ad,1,-1,	0x04af,1,-1,	
0x04b1,1,-1,	0x04b3,1,-1,	0x04b5,1,-1,	0x04b7,1,-1,	0x04b9,1,-1,	0x04bb,1,-1,	0x04bd,1,-1,	0x04bf,1,-1,	
0x04c2,1,-1,	0x04c4,1,-1,	0x04c8,1,-1,	0x04cc,1,-1,	0x04d1,1,-1,	0x04d3,1,-1,	0x04d5,1,-1,	0x04d7,1,-1,	
0x04d9,1,-1,	0x04db,1,-1,	0x04dd,1,-1,	0x04df,1,-1,	0x04e1,1,-1,	0x04e3,1,-1,	0x04e5,1,-1,	0x04e7,1,-1,	
0x04e9,1,-1,	0x04eb,1,-1,	0x04ef,1,-1,	0x04f1,1,-1,	0x04f3,1,-1,	0x04f5,1,-1,	0x04f9,1,-1,	0x0561,38,-48,	
0x10d0,38,-48,	0x1e01,1,-1,	0x1e03,1,-1,	0x1e05,1,-1,	0x1e07,1,-1,	0x1e09,1,-1,	0x1e0b,1,-1,	0x1e0d,1,-1,	
0x1e0f,1,-1,	0x1e11,1,-1,	0x1e13,1,-1,	0x1e15,1,-1,	0x1e17,1,-1,	0x1e19,1,-1,	0x1e1b,1,-1,	0x1e1d,1,-1,	
0x1e1f,1,-1,	0x1e21,1,-1,	0x1e23,1,-1,	0x1e25,1,-1,	0x1e27,1,-1,	0x1e29,1,-1,	0x1e2b,1,-1,	0x1e2d,1,-1,	
0x1e2f,1,-1,	0x1e31,1,-1,	0x1e33,1,-1,	0x1e35,1,-1,	0x1e37,1,-1,	0x1e39,1,-1,	0x1e3b,1,-1,	0x1e3d,1,-1,	
0x1e3f,1,-1,	0x1e41,1,-1,	0x1e43,1,-1,	0x1e45,1,-1,	0x1e47,1,-1,	0x1e49,1,-1,	0x1e4b,1,-1,	0x1e4d,1,-1,	
0x1e4f,1,-1,	0x1e51,1,-1,	0x1e53,1,-1,	0x1e55,1,-1,	0x1e57,1,-1,	0x1e59,1,-1,	0x1e5b,1,-1,	0x1e5d,1,-1,	
0x1e5f,1,-1,	0x1e61,1,-1,	0x1e63,1,-1,	0x1e65,1,-1,	0x1e67,1,-1,	0x1e69,1,-1,	0x1e6b,1,-1,	0x1e6d,1,-1,	
0x1e6f,1,-1,	0x1e71,1,-1,	0x1e73,1,-1,	0x1e75,1,-1,	0x1e77,1,-1,	0x1e79,1,-1,	0x1e7b,1,-1,	0x1e7d,1,-1,	
0x1e7f,1,-1,	0x1e81,1,-1,	0x1e83,1,-1,	0x1e85,1,-1,	0x1e87,1,-1,	0x1e89,1,-1,	0x1e8b,1,-1,	0x1e8d,1,-1,	
0x1e8f,1,-1,	0x1e91,1,-1,	0x1e93,1,-1,	0x1e95,1,-1,	0x1ea1,1,-1,	0x1ea3,1,-1,	0x1ea5,1,-1,	0x1ea7,1,-1,	
0x1ea9,1,-1,	0x1eab,1,-1,	0x1ead,1,-1,	0x1eaf,1,-1,	0x1eb1,1,-1,	0x1eb3,1,-1,	0x1eb5,1,-1,	0x1eb7,1,-1,	
0x1eb9,1,-1,	0x1ebb,1,-1,	0x1ebd,1,-1,	0x1ebf,1,-1,	0x1ec1,1,-1,	0x1ec3,1,-1,	0x1ec5,1,-1,	0x1ec7,1,-1,	
0x1ec9,1,-1,	0x1ecb,1,-1,	0x1ecd,1,-1,	0x1ecf,1,-1,	0x1ed1,1,-1,	0x1ed3,1,-1,	0x1ed5,1,-1,	0x1ed7,1,-1,	
0x1ed9,1,-1,	0x1edb,1,-1,	0x1edd,1,-1,	0x1edf,1,-1,	0x1ee1,1,-1,	0x1ee3,1,-1,	0x1ee5,1,-1,	0x1ee7,1,-1,	
0x1ee9,1,-1,	0x1eeb,1,-1,	0x1eed,1,-1,	0x1eef,1,-1,	0x1ef1,1,-1,	0x1ef3,1,-1,	0x1ef5,1,-1,	0x1ef7,1,-1,	
0x1ef9,1,-1,	0x1f00,8,8,		0x1f10,6,8,		0x1f20,8,8,		0x1f30,8,8,		0x1f40,6,8,		0x1f51,1,8,		0x1f53,1,8,	
0x1f55,1,8,		0x1f57,1,8,		0x1f60,8,8,		0x1f80,8,8,		0x1f90,8,8,		0x1fa0,8,8,		0x1fb0,2,8,		0x1fd0,2,8,	
0x1fe0,2,8,		0x24d0,26,-26,	0xff41,26,-32,
			};
			count = sizeof(map) / sizeof(map[0]);
			return map;
		}
	};



	// Compare semantics for simple values
	struct SCompareString
	{
		static int Compare(char32_t a, char32_t b)
		{
			return a - b;
		}

		template <typename T>
		static int Compare(const T& a, const T& b)
		{
			return CEncoding<T>::Compare(a, b);
		}

		template <typename T>
		static bool AreEqual(const T& a, const T& b)
		{
			return Compare(a, b) == 0;
		}
	};

	// Compare semantics for simple values
	struct SCompareStringI
	{
		static int Compare(char32_t a, char32_t b)
		{
			return SCaseConversion::ToUpper(a) - SCaseConversion::ToUpper(b);
		}

		template <typename T>
		static int Compare(const T& a, const T& b)
		{
			return CEncoding<T>::CompareI(a, b);
		}

		template <typename T>
		static bool AreEqual(const T& a, const T& b)
		{
			return Compare(a, b) == 0;
		}
	};

	struct SCase
	{
		typedef SStorageValue TStorage;
		typedef SCompareString TCompare;
	};

	struct SCaseI
	{
		typedef SStorageValue TStorage;
		typedef SCompareStringI TCompare;
	};


} // namespace

#endif  // __simplelib_caseconversion_h__
