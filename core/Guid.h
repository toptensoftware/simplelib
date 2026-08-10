#pragma once

#include <string.h>
#include "String.h"

namespace SimpleLib
{

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4996)
#endif

#pragma pack(1)
struct Guid
{
	uint32_t  Data1 = 0;
	uint16_t Data2 = 0;
	uint16_t Data3 = 0;
	uint8_t  Data4[8] = { 0 };

	bool operator==(const Guid& other) const
	{
		return memcmp(this, &other, sizeof(*this)) == 0;
	}
	bool operator!=(const Guid& other) const
	{
		return memcmp(this, &other, sizeof(*this)) != 0;
	}

	String ToString()
	{
		return String::Format("%.8x-%.4x-%.4x-%.2x%.2x-%.2x%.2x%.2x%.2x%.2x%.2x",
			Data1, Data2, Data3, Data4[0], Data4[1], Data4[2], Data4[3], Data4[4], Data4[5], Data4[6], Data4[7]);
	}

	String ToStringCompact()
	{
		return String::Format("%.8x%.4x%.4x%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x",
			Data1, Data2, Data3, Data4[0], Data4[1], Data4[2], Data4[3], Data4[4], Data4[5], Data4[6], Data4[7]);
	}

	static bool ParseCompact(const char* in, Guid& uuid)
	{
		int 		i;
		const char* cp;
		char		buf[33];

		if (strlen(in) != 32)
			return false;

		for (int i = 0; i < 32; i++) {
			if (!isxdigit(in[i]))
				return false;
		}

		strcpy(buf, in);
		buf[8] = '\0';
		uuid.Data1 = (uint32_t)strtoul(buf, NULL, 16);

		strcpy(buf, in + 8);
		buf[4] = '\0';
		uuid.Data2 = (uint16_t)strtoul(buf, NULL, 16);

		strcpy(buf, in + 12);
		buf[4] = '\0';
		uuid.Data3 = (uint16_t)strtoul(buf, NULL, 16);

		cp = in + 16;
		buf[2] = 0;
		for (i = 0; i < 8; i++) {
			buf[0] = *cp++;
			buf[1] = *cp++;
			uuid.Data4[i] = (uint8_t)strtoul(buf, NULL, 16);
		}

		return true;
	}

	static bool Parse(const char* in, Guid& uuid)
	{
        char buf[33];
        buf[32] = '\0';
        int pos = 0;

        // Leading '{'
        if (*in == '{')
            in++;

        // Copy digits
        while (*in && pos < 32)
        {
            if (isxdigit(in[0]))
                buf[pos] = in[0];
            else if (in[0] != '-' && in[0] != '_')
                return false;
            pos++;
        }

        // Must have 32 digits
        if (pos != 32)
            return false;

        // Trailing '}'
        if (*in == '}')
            in++;
        
        // Check eof
        if (*in != '\0')
            return false;
        
        // Parse it
        return ParseCompact(buf, uuid);
	}

    static Guid Null;
};

inline Guid Guid::Null;
#pragma pack()

#ifdef _MSC_VER
#pragma warning(pop)
#endif

}
