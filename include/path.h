#ifndef __simplelib_path_h__
#define __simplelib_path_h__

#ifndef _WIN32
#define SIMPLELIB_POSIX_PATHS
#endif

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "semantics.h"

#ifdef _MSC_VER
#include "direct.h"
#include <windows.h>
#endif

namespace SimpleLib
{
    template <typename T>
    class CPath
    {
    public:
        // Simple join of two paths ensuring one directory separator
        // between them
        static CString<T> Join(const T* a, const T* b)
        {
            // If either empty, result is the other
            if (CString<T>::IsNullOrEmpty(a))
                return b;
            if (CString<T>::IsNullOrEmpty(b))
                return a;

            // Start with a
            CStringBuilder<T> sb;
            sb.Append(a);

            // Make sure there's a separator
            if (!IsDirectorySeparator(a[SChar<T>::Length(a)-1]))
                sb.Append(DirectorySeparator);

            // Skip leading separator on b
            if (IsDirectorySeparator(b[0]))
                b++;
            
            // Join
            sb.Append(b);

            // Return string
            return sb.ToString();
        }

        // Get the directory name from a path
        // (ie: everything up to  last separator)
        static CString<T> GetDirectoryName(const T* path)
        {
            CString<T> str(path);
            int lastSep = str.LastIndexOfAny(GetDirectorySeparators());
            int prefix = GetPrefixLength(path);

            if (lastSep < prefix)
                return (T*)nullptr;

            if (str.GetLength() == prefix + 1)
                return (T*)nullptr;

            if (lastSep == prefix)
                return str.SubString(0, lastSep + (prefix == 2 ? 1 : 0));

            if (lastSep < 0)
                return (T*)nullptr;
            else
                return str.SubString(0, lastSep);
        }

        // Get the filename component from a path
        // (ie: everything after the last separator)
        static CString<T> GetFileName(const T* path)
        {
            CString<T> str(path);
            int lastSep = str.LastIndexOfAny(GetDirectorySeparators());
            if (lastSep < 0)
                return path;
            else
                return str.SubString(lastSep+1);
        }

        // Get the filename component from a path
        // (ie: everything after the last separator)
        static CString<T> GetFileNameWithoutExtension(const T* path)
        {
            CString<T> str(path);
            int lastSep = str.LastIndexOfAny(GetDirectorySeparators());
            if (lastSep < 0)
                return path;

            int extPos = str.LastIndexOf(".");
            if (extPos > lastSep)
                return str.SubString(lastSep+1, extPos - (lastSep + 1));
            else
                return str.SubString(lastSep+1);
        }

        // Change (or add) the a path's file extension
        static CString<T> ChangeExtension(const T* path, const T* newExtension)
        {
            const T* ext = FindExtension(path);

            CStringBuilder<T> sb;
            if (ext == nullptr)
                sb.Append(path);
            else
                sb.Append(path, ext - path);

            if (newExtension[0] != '.')
                sb.Append('.');
            sb.Append(newExtension);
            return sb.ToString();
        }

        // Get the filename component from a path
        // (ie: everything after the last separator)
        static CString<T> GetExtension(const T* path)
        {
            return FindExtension(path);
        }

        // Returns a direct pointer to a path's file extension
        // (or nullptr if none)
        static const T* FindExtension(const T* path)
        {
            CString<T> str(path);
            int lastSep = str.LastIndexOfAny(GetDirectorySeparators());
            int lastDot = str.LastIndexOfAny(".");
            if (lastDot > lastSep)
                return path + lastDot;
            else
                return nullptr;
        }

        // Canonicalize a path
        static CString<T> Canonicalize(const T* path)
        {
            CString<T> strPath(path);

            CVector<CString<T>> parts;
            strPath.Split(GetDirectorySeparators(), true, parts);

            for (int i=0; i<parts.GetCount(); i++)
            {
                // Collapse // to / (unless at start of path)
                // Also, keep empty if at end to maintain trailing slash
                if (parts[i].IsEmpty() && i > 1 && i != parts.GetCount() - 1)
                {
                    parts.RemoveAt(i);
                    i--;
                    continue;
                }

                if (parts[i].IsEqualTo("."))
                {
                    parts.RemoveAt(i);
                    i--;
                    continue;
                }
                if (parts[i].IsEqualTo(".."))
                {
                    parts.RemoveAt(i);
                    i--;
                    if (parts.GetCount() > 0)
                    {
                        parts.RemoveAt(i);
                        i--;
                    }
                }
            }

            return CString<T>::Join(parts, DirectorySeparator);
        }

        // Combine two paths
        static CString<T> Combine(const T* base, const T* path)
        {
            assert(!CString<T>::IsNullOrEmpty(base));

            if (path != nullptr && IsDirectorySeparator(path[0]))
            {
                int prefix = GetPrefixLength(base);
                return Canonicalize(Join(CString<T>(base, prefix), path));
            }
            else
                return Canonicalize(Join(base, path));
        }

        // Get the current directory
        static CString<T> GetCurrentDirectory()
        {
#ifdef _MSC_VER
            char16_t* psz = (char16_t*)_wgetcwd(nullptr, 0);
            CString<T> cwd= Encode<T>(psz);
            free(psz);
            return cwd;
#else
#endif
        }

        // Get the full path of a path
        static CString<T> GetFullPath(const T* path)
        {
#ifdef _MSC_VER
            // Special case for "C:relative/path"
            int prefix = GetPrefixLength(path);
            if (prefix == 2 && !IsDirectorySeparator(path[2]))
            {
                wchar_t base[512];
                wchar_t drive[3] = { (wchar_t)path[0], ':', '\0' };
                GetFullPathNameW(drive, 512, base, NULL);
                return Combine(Encode<T>((char16_t*)base), path + 2);
            }
#endif

            return Combine(GetCurrentDirectory(), path);
        }

        static bool IsFullyQualified(const T* psz)
        {
#if _WIN32
            int prefix = GetPrefixLength(psz);
            if (prefix == 0)
                return false;
            return prefix > 2 || IsDirectorySeparator(psz[prefix]);
#else
            return IsDirectorySeparator(psz[0]);
#endif
        }

        static int GetPrefixLength(const T* psz)
        {
#ifdef _WIN32
            if (psz == nullptr)
                return 0;
            if (psz[0] == '\0')
                return 0;

            // Drive letter
            if (psz[1] == ':')
                return 2;

            // Unc
            if (IsDirectorySeparator(psz[0]) && IsDirectorySeparator(psz[1]))
            {
                const T* p = psz + 2;
                int seps = 0;
                while (*p && seps < 2)
                {
                    if (IsDirectorySeparator(*p++))
                        seps++;
                }

                if (IsDirectorySeparator(p[-1]))
                    p--;

                return p - psz;
            }
#endif
            return 0;
        }

        static bool IsDirectorySeparator(T ch)
        {
#ifdef _WIN32
            return ch == '\\' || ch == '/';
#else
            return ch == '/';
#endif
        }

        inline static const T* GetDirectorySeparators()
        {
#ifdef _WIN32
            return  { '\\', '/', '\0' };
#else
            static T seps[] = { '/', '\0' };
            return seps;
#endif
        }

#ifdef _WIN32
        static const T DirectorySeparator = '\\';
        static const T PathSeparator =';';
#else
        static const T DirectorySeparator ='/';
        static const T PathSeparator =':';
#endif
    };

} // namespace

#endif  // __simplelib_path_h__