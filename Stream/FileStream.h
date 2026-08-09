#pragma once

#include <io.h>

#include "Stream.h"
#include "../Core/Encoding.h"

namespace SimpleLib
{


class FileStream : public Stream
{
public:

    FileStream()
    {
        m_pFile = nullptr;
    }

    virtual ~FileStream()
    {
        Close();
    }

    int Create(const char* pszFileName)
    {
        return Open(pszFileName, "wb+");
    }

    int Open(const char* pszFileName, const char* pszMode)
    {
        assert(m_pFile==nullptr);
    	int err = _wfopen_s(&m_pFile, Encode<wchar_t>(pszFileName), Encode<wchar_t>(pszMode));
        if (err)
        {
            m_pFile = nullptr;
            return err;
        }
        return 0;
    }

    void Close()
    {
        if (m_pFile!=nullptr)
        {
            fclose(m_pFile);
            m_pFile = nullptr;
        }
    }

    virtual int Read(void* pv, uint32_t cb, uint32_t* pcb = nullptr) override
    {
        assert(m_pFile!=nullptr);
        errno = 0;
        uint32_t cbRead = (uint32_t)fread(pv, 1, cb, m_pFile);
        if (errno!=0)
            return errno;

        if (pcb)
        {
            *pcb = cbRead;
        }
        return 0;
    }

    virtual int Write(const void* pv, uint32_t cb, uint32_t* pcb = nullptr) override
    {
        assert(m_pFile!=nullptr);
        errno = 0;
        uint32_t cbWrite = (uint32_t)fwrite(pv, 1, cb, m_pFile);
        if (errno!=0)
            return errno;
        if (pcb)
        {
            *pcb=cbWrite;
        }
        return 0;
    }

    virtual int Seek(int64_t offset, int origin) override
    {
        assert(m_pFile!=nullptr);
    	return _fseeki64(m_pFile, offset, origin);
    }

    virtual int64_t Tell() override
    {
        assert(m_pFile!=nullptr);
        return _ftelli64(m_pFile);
    }

    virtual int64_t Length() override
    {
        assert(m_pFile!=nullptr);
        int64_t save = Tell();
        Seek(0, SEEK_END);
        int64_t length = Tell();
        Seek(save, SEEK_SET);
        return length;
    }

    virtual int SetLength() override
    {
        assert(m_pFile!=nullptr);
    	return _chsize_s(_fileno(m_pFile), Tell());
    }

private:
	FILE* m_pFile;
};



}
