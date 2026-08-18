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

    virtual bool IsOpen() = 0;
	virtual int Read(void* pv, size_t cb, size_t* pcb = nullptr) = 0;
	virtual int Write(const void* pv, size_t cb, size_t* pcb = nullptr) = 0;
	virtual int Seek(int64_t offset, int origin = SEEK_SET) = 0;
	virtual int64_t Tell() = 0;
	virtual int64_t GetLength() = 0;
	virtual int Truncate() = 0;
    virtual int Flush() { return 0; };

    bool IsEof() { return Tell() == GetLength(); }


    template <typename T>
    int WriteValue(const T& value)
    {
        return Write(&value, sizeof(value));
    }

    template <typename T>
    int ReadValue(T& value)
    {
        return Read(&value, sizeof(value));
    }

    template <typename T>
    T ReadValue()
    {
        T value;
        Read(&value, sizeof(value));
        return value;
    }

    int Write(const char* psz)
    {
        if (!psz)
            return 0;
        return Write(psz, SChar<char>::Length(psz));
    }

    int Write(const wchar_t* psz)
    {
        if (!psz)
            return 0;

        return Write(psz, SChar<wchar_t>::Length(psz) * sizeof(wchar_t*));
    }

    void WriteLenString(const char* psz)
    {
        if (psz == nullptr)
        {
            WriteValue<uint32_t>(0);
            return;
        }

        // Convert to utf-8
        size_t length = SChar<char>::Length(psz);

        // Write it
        WriteValue<uint32_t>((uint32_t)length);
        Write(psz, length);
    }

    String ReadLenString()
    {
        // Read length
        uint32_t utf8Length = ReadValue<uint32_t>();
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
            size_t cb;
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
            size_t cb;
            RIFE(src.Read(buf, blockLength, &cb));

            // Write
            RIFE(dest.Write(buf, blockLength, nullptr));

            length -= blockLength;
        }

        return 0;
    }

};

}