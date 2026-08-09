#pragma once

namespace SimpleLib
{

#define RIFE(x) { int r = (x); if (r!=0) return r; }

class Stream
{
public:
    Stream()
    {
    }

    virtual ~Stream()
    {
    }

	virtual int Read(void* pv, uint32_t cb, uint32_t* pcb=nullptr) = 0;
	virtual int Write(const void* pv, uint32_t cb, uint32_t* pcb=nullptr) = 0;
	virtual int Seek(int64_t offset, int origin=SEEK_SET) = 0;
	virtual int64_t Tell() = 0;
	virtual int64_t Length() = 0;
	virtual int SetLength() = 0;

    bool IsEof() { return Tell() == Length(); }


    void WriteInt32(int32_t value)
    {
        Write(&value, sizeof(value));
    }

    int32_t ReadInt32()
    {
        int32_t value;
        Read(&value, sizeof(value));
        return value;
    }

    void WriteUInt32(uint32_t value)
    {
        Write(&value, sizeof(value));
    }

    uint32_t ReadUInt32()
    {
        uint32_t value;
        Read(&value, sizeof(value));
        return value;
    }

/*
    void WriteInt32BE(int32_t value)
    {
        Reverse(value);
        Write(&value, sizeof(value));
    }

    int ReadInt32BE()
    {
        int value;
        Read(&value, sizeof(value));
        Reverse(value);
        return value;
    }
*/

    void Stream::WriteString(const char* psz)
    {
        if (psz == nullptr)
        {
            WriteInt32(0);
            return;
        }

        // Convert to utf-8
        uint32_t length = (uint32_t)strlen(psz);

        // Write it
        WriteUInt32(length);
        Write(psz, length);
    }

    String ReadString()
    {
        // Read length
        uint32_t utf8Length = ReadUInt32();
        if (utf8Length == 0)
            return "";

        // Get buffer
        StringBuilder<char> buf;
        char* pBuf = buf.Reserve(utf8Length);

        // Read it
        Read(pBuf, utf8Length);

        // Return string
        return String(pBuf, utf8Length);
    }

    static int Copy(Stream& dest, Stream& src)
    {
        char buf[4096];
        while (!src.IsEof())
        {
            // Read 
            uint32_t cb;
            RIFE(src.Read(buf, sizeof(buf), &cb));

            // Write
            RIFE(dest.Write(buf, cb, nullptr));
        }

        return 0;
    }

    static int Copy(Stream& dest, Stream& src, int32_t length)
    {
        char buf[4096];
        while (length)
        {
            int32_t blockLength = sizeof(buf);
            if (length < blockLength)
                blockLength = length;

            // Read 
            uint32_t cb;
            RIFE(src.Read(buf, blockLength, &cb));

            // Write
            RIFE(dest.Write(buf, blockLength, nullptr));

            length -= blockLength;
        }

        return 0;
    }

};

}