#pragma once

#include "../Core/Encoding.h"

#ifdef _WIN32
#include <Windows.h>
#endif

namespace SimpleLib
{

struct Stat
{
	int64_t size;
	int64_t mtime;
};

class FileSystem
{
public:
    static bool Exists(const char* pszFileName)
    {
        return _waccess(Encode<wchar_t>(pszFileName), 0) == 0;
    }

    static String<char> GetTempDirectory()
    {
        wchar_t sz[MAX_PATH];
        GetTempPathW(MAX_PATH, sz);
        return Encode<char>(sz);
    }

    static int Rename(const char* pszOldName, const char* pszNewName)
    {
        return _wrename(Encode<wchar_t>(pszOldName), Encode<wchar_t>(pszNewName));
    }

    static int Unlink(const char* pszFileName)
    {
        return _wunlink(Encode<wchar_t>(pszFileName));
    }

    static int Stat(const char* pszFileName, Stat* stat)
    {
        struct _stat64 s;
        int retv = _wstati64(Encode<wchar_t>(pszFileName), &s);
        stat->size = s.st_size;
        stat->mtime = s.st_mtime;
        return retv;
    }

    static int MakeDirectory(const char* pszDirName)
    {
        return _wmkdir(Encode<wchar_t>(pszDirName));
    }
};


}
