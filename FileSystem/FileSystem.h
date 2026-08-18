    #pragma once

#include "../Platform/Platform.h"

namespace SimpleLib
{

class FileSystem
{
public:
    static bool Exists(const char* pszFileName)
    {
        return Platform::FileExists(pszFileName);
    }

    static String GetTempDirectory()
    {
        return Platform::GetTempDirectory();
    }

    static int Rename(const char* pszOldName, const char* pszNewName)
    {
        return Platform::Rename(pszOldName, pszNewName);
    }

    static int Unlink(const char* pszFileName)
    {
        return Platform::Unlink(pszFileName);
    }

    static int Stat(const char* pszFileName, Stat* stat)
    {
        return Platform::Stat(pszFileName, stat);
    }

    static int MakeDirectory(const char* pszDirName)
    {
        return Platform::MakeDirectory(pszDirName);
    }
};


}
