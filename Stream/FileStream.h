#pragma once

#include <io.h>

#include "Stream.h"
#include "../Core/Encoding.h"

namespace SimpleLib
{

/*
struct u_stat
{
	int64_t size;
	int64_t mtime;
};

bool u_fileexists(const char* pszFileName)
{
	return _waccess(Encode<wchar_t>(pszFileName), 0) == 0;
}

String<char> u_tmpdir(wchar_t* psz, int cch)
{
    wchar_t sz[MAX_PATH];
	GetTempPathW(MAX_PATH, sz);
    return Encode<char>(sz);
}

int u_rename(const char* pszOldName, const char* pszNewName)
{
	return _wrename(Encode<wchar_t>(pszOldName), Encode<wchar_t>(pszNewName));
}

int u_unlink(const char* pszFileName)
{
	return _wunlink(Encode<wchar_t>(pszFileName));
}

int u_stat(const char* pszFileName, struct u_stat* stat)
{
	struct _stat64 s;
	int retv = _wstati64(Encode<wchar_t>(pszFileName), &s);
	stat->size = s.st_size;
	stat->mtime = s.st_mtime;
	return retv;
}

int u_mkdir(const char* pszDirName)
{
	return _wmkdir(Encode<wchar_t>(pszDirName));
}
*/


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
