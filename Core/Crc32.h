#pragma once

namespace SimpleLib
{

class Crc32
{
public:
    Crc32()
    {
        Reset();
    }

    void Reset()
    {
        // Make sure initialised
        InitTable();
        dwCRC = 0xFFFFFFFF;
    }

    void Update(unsigned char byte)
    {
        dwCRC = (dwCRC >> 8) ^ g_dwCRCTable[(dwCRC & 0xFF) ^ byte];
    }

    void Update(const void* pbDataIn, size_t cbData)
    {
        const unsigned char* pbData = (const unsigned char*)pbDataIn;
        while (cbData--)
        {
            dwCRC = (dwCRC >> 8) ^ g_dwCRCTable[(dwCRC & 0xFF) ^ *pbData++];
        }
    }

    uint32_t Finish()
    {
        // Exclusive OR the result with the beginning value.
        return dwCRC ^ 0xffffffff;
    }

    static uint32_t Calculate(const void* pbData, size_t cbData)
    {
        Crc32 crc;
        crc.Update(pbData, cbData);
        return crc.Finish();
    }

    uint32_t dwCRC = 0xFFFFFFFF;


    inline static uint32_t g_dwCRCTable[256];
    inline static bool  g_bInitialized = false;

    // Reflection is a requirement for the official CRC-32 standard.
    // You can create CRCs without it, but they won't conform to the standard.
    static uint32_t Reflect(uint32_t ref, char ch)
    {// Used only by _Init()

        uint32_t value(0);

        // Swap bit 0 for bit 7
        // bit 1 for bit 6, etc.
        for (int i = 1; i < (ch + 1); i++)
        {
            if (ref & 1)
                value |= 1 << (ch - i);
            ref >>= 1;
        }
        return value;
    }

    static void InitTable()
    {
        // Quit if already initialised
        if (g_bInitialized)
            return;

        g_bInitialized = true;

        // This is the official polynomial used by CRC-32
        // in PKZip, WinZip and Ethernet.
        uint32_t ulPolynomial = 0x04c11db7;

        // 256 values representing ASCII character codes.
        for (int i = 0; i <= 0xFF; i++)
        {
            g_dwCRCTable[i] = Reflect(i, 8) << 24;
            for (int j = 0; j < 8; j++)
                g_dwCRCTable[i] = (g_dwCRCTable[i] << 1) ^ (g_dwCRCTable[i] & (1 << 31) ? ulPolynomial : 0);
            g_dwCRCTable[i] = Reflect(g_dwCRCTable[i], 32);
        }
    }
};



}