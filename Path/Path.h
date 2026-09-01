#pragma once

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "../core/string.h"
#include "../core/encoding.h"

#ifdef _MSC_VER
#include <direct.h>
#include <windows.h>
#else
#include <unistd.h>
#endif

namespace SimpleLib
{

struct SPathSemanticsWindows
{
    static bool IsDirectorySeparator(char ch)
    {
        return ch == '\\' || ch == '/';
    }

    inline static const char* GetDirectorySeparators()
    {
        static char seps[] = { '\\', '/', '\0' };
        return  seps;
    }

    static const char DirectorySeparator = '\\';
    static const char PathSeparator =';';

    // Windows understands "C:" drive letters and "\\server\share" UNC roots
    static constexpr bool SupportsVolumeNames = true;

    typedef SCaseI FileSystemCase;

};

struct SPathSemanticsPosix
{
    static bool IsDirectorySeparator(char ch)
    {
        return ch == '/';
    }

    inline static const char* GetDirectorySeparators()
    {
        static char seps[] = { '/', '\0' };
        return seps;
    }

    static const char DirectorySeparator ='/';
    static const char PathSeparator =':';

    // Posix has a single "/" root and no volume names
    static constexpr bool SupportsVolumeNames = false;

    typedef SCase FileSystemCase;
};

#ifdef _WIN32
typedef SPathSemanticsWindows SPathSemanticsAuto;
#else
typedef SPathSemanticsPosix SPathSemanticsAuto;
#endif


template <typename SPathSemantics = SPathSemanticsAuto>
class Path
{
public:
    // Simple join of two paths ensuring one directory separator
    // between them
    static String Join(const char* a, const char* b)
    {
        // If either empty, result is the other
        if (String::IsNullOrEmpty(a))
            return b;
        if (String::IsNullOrEmpty(b))
            return a;

        // Start with a
        StringBuilder<char> sb;
        sb.Append(a);

        // Make sure there's a separator
        if (!SPathSemantics::IsDirectorySeparator(a[SChar<char>::Length(a)-1]))
            sb.Append(SPathSemantics::DirectorySeparator);

        // Skip leading separator on b
        if (SPathSemantics::IsDirectorySeparator(b[0]))
            b++;
        
        // Join
        sb.Append(b);

        // Return string
        return sb.ToString();
    }

    // Get the directory part of a path (everything up to, but not including,
    // the last separator). Returns null if the path has no directory part or
    // is itself a root. The root separator is kept when the parent is a root
    // (eg: "/foo" -> "/", "C:\foo" -> "C:\").
    static String GetDirectoryName(const char* path)
    {
        if (String::IsNullOrEmpty(path))
            return (char*)nullptr;

        String str(path);
        int prefix = GetPrefixLength(path);
        int lastSep = str.LastIndexOfAny(SPathSemantics::GetDirectorySeparators());

        // No separator past the volume prefix => no directory part
        if (lastSep < prefix)
            return (char*)nullptr;

        // Path is a bare root ("<prefix>/") => no parent
        if (lastSep == prefix && str.GetLength() == prefix + 1)
            return (char*)nullptr;

        // Parent is the root itself => keep the separator
        if (lastSep == prefix)
            return str.SubString(0, prefix + 1);

        return str.SubString(0, lastSep);
    }

    // Get the filename part of a path (everything after the last separator).
    static String GetFileName(const char* path)
    {
        if (String::IsNullOrEmpty(path))
            return (char*)nullptr;

        String str(path);
        int lastSep = str.LastIndexOfAny(SPathSemantics::GetDirectorySeparators());
        return str.SubString(lastSep + 1);
    }

    // Get the filename part of a path with its extension removed.
    static String GetFileNameWithoutExtension(const char* path)
    {
        if (String::IsNullOrEmpty(path))
            return (char*)nullptr;

        String str(path);
        int lastSep = str.LastIndexOfAny(SPathSemantics::GetDirectorySeparators());
        int extPos = str.LastIndexOf(".");
        if (extPos > lastSep)
            return str.SubString(lastSep + 1, extPos - (lastSep + 1));
        else
            return str.SubString(lastSep + 1);
    }

    // Change (or add) the a path's file extension
    static String ChangeExtension(const char* path, const char* newExtension)
    {
        const char* ext = FindExtension(path);

        StringBuilder<char> sb;
        if (ext == nullptr)
            sb.Append(path);
        else
            sb.Append(path, (int)(ext - path));

        if (newExtension[0] != '.')
            sb.Append('.');
        sb.Append(newExtension);
        return sb.ToString();
    }

    // Get the filename component from a path
    // (ie: everything after the last separator)
    static String GetExtension(const char* path)
    {
        return FindExtension(path);
    }

    // Returns a direct pointer to a path's file extension
    // (or nullptr if none)
    static const char* FindExtension(const char* path)
    {
        String str(path);
        int lastSep = str.LastIndexOfAny(SPathSemantics::GetDirectorySeparators());
        int lastDot = str.LastIndexOfAny(".");
        if (lastDot > lastSep)
            return path + lastDot;
        else
            return nullptr;
    }

    // Lexically canonicalize a path: collapse '.', '..', and redundant
    // separators without touching the filesystem. Any volume/UNC prefix and a
    // leading root separator are preserved verbatim; a trailing separator is
    // preserved if the input had one. A path that reduces to nothing becomes ".".
    static String Canonicalize(const char* path)
    {
        if (String::IsNullOrEmpty(path))
            return path;

        String strPath(path);
        int len = strPath.GetLength();

        // Split off any volume / UNC prefix (eg: "C:" or "\\server\share").
        int prefix = GetPrefixLength(path);

        // Is what follows the prefix rooted (an absolute path)?
        bool rooted = prefix < len && SPathSemantics::IsDirectorySeparator(path[prefix]);

        // Did the caller supply a trailing separator we should restore? (but not
        // when the whole path is just the root separator itself)
        bool trailingSep = len > prefix
            && SPathSemantics::IsDirectorySeparator(path[len - 1])
            && !(rooted && len == prefix + 1);

        // Split the body into segments, dropping empties (from "//") and ".".
        List<String> segs;
        strPath.SubString(prefix).Split(SPathSemantics::GetDirectorySeparators(), false, segs);

        List<String> out;
        for (int i = 0; i < segs.GetCount(); i++)
        {
            if (segs[i].IsEqualTo("."))
                continue;

            if (segs[i].IsEqualTo(".."))
            {
                if (out.GetCount() > 0 && !out[out.GetCount() - 1].IsEqualTo(".."))
                    out.RemoveAt(out.GetCount() - 1);
                else if (!rooted)
                    out.Add("..");
                // else: ".." at the root has nowhere to go - drop it
                continue;
            }

            out.Add(segs[i]);
        }

        // Reassemble
        StringBuilder<char> sb;
        if (prefix > 0)
            sb.Append(strPath.SubString(0, prefix));
        if (rooted)
            sb.Append(SPathSemantics::DirectorySeparator);
        for (int i = 0; i < out.GetCount(); i++)
        {
            if (i > 0)
                sb.Append(SPathSemantics::DirectorySeparator);
            sb.Append(out[i]);
        }
        if (trailingSep && out.GetCount() > 0)
            sb.Append(SPathSemantics::DirectorySeparator);

        if (sb.GetLength() == 0)
            return ".";

        return sb.ToString();
    }

    // Join 'path' onto 'base' and canonicalize the result. If 'path' is rooted
    // it replaces 'base' (keeping only base's volume prefix, matching .NET's
    // Path.Combine). Unlike .NET, '.' and '..' segments in the result are
    // collapsed.
    static String Combine(const char* base, const char* path)
    {
        if (String::IsNullOrEmpty(base))
            return Canonicalize(path);

        if (path != nullptr && SPathSemantics::IsDirectorySeparator(path[0]))
        {
            // 'path' is rooted - anchor it to base's volume prefix only
            return Canonicalize(Join(String(base, GetPrefixLength(base)), path));
        }

        return Canonicalize(Join(base, path));
    }

    // Get the process's current working directory.
    static String GetCurrentDirectory()
    {
#ifdef _MSC_VER
        wchar_t* psz = _wgetcwd(nullptr, 0);
        String cwd = psz ? Encode<char>(psz) : String();
        free(psz);
        return cwd;
#else
        char* psz = getcwd(nullptr, 0);
        String cwd = psz ? String(psz) : String();
        free(psz);
        return cwd;
#endif
    }

    // Resolve 'path' to a fully qualified, canonicalized path, using the
    // current directory (and, for a drive-relative path like "C:foo", the
    // current directory of that drive) as the anchor. Does not touch the
    // filesystem beyond reading those.
    static String GetFullPath(const char* path)
    {
        if (String::IsNullOrEmpty(path))
            return GetCurrentDirectory();

#ifdef _MSC_VER
        // Drive-relative ("C:foo"): only the OS knows drive C's current
        // directory, so ask it for the drive's full path and combine.
        if (GetPrefixLength(path) == 2 && path[1] == ':' &&
            !SPathSemantics::IsDirectorySeparator(path[2]))
        {
            wchar_t drive[3] = { (wchar_t)path[0], L':', L'\0' };

            DWORD needed = GetFullPathNameW(drive, 0, nullptr, nullptr);
            if (needed != 0)
            {
                StringBuilder<wchar_t> wbuf;
                DWORD got = GetFullPathNameW(drive, needed, wbuf.GetBuffer((int)needed), nullptr);
                if (got != 0 && got < needed)
                    return Combine(Encode<char>(wbuf.SyncLength().sz()), path + 2);
            }
        }
#endif

        return Combine(GetCurrentDirectory(), path);
    }

    // True if the path is absolute - it needs nothing from the current
    // directory (or, on Windows, the current drive) to locate it.
    static bool IsFullyQualified(const char* psz)
    {
        if (String::IsNullOrEmpty(psz))
            return false;

        if constexpr (SPathSemantics::SupportsVolumeNames)
        {
            int prefix = GetPrefixLength(psz);
            if (prefix == 0)
                return false;

            // A UNC root (prefix > 2) is always absolute; a bare drive ("C:")
            // still needs a root separator to be absolute ("C:\").
            return prefix > 2 || SPathSemantics::IsDirectorySeparator(psz[prefix]);
        }

        return SPathSemantics::IsDirectorySeparator(psz[0]);
    }

    // Length of the leading volume specifier that must be preserved verbatim
    // during path manipulation: a drive ("C:" -> 2) or a UNC share
    // ("\\server\share" -> 14) on Windows, always 0 on Posix. Does not include
    // any root separator that follows.
    static int GetPrefixLength(const char* psz)
    {
        if (psz == nullptr || psz[0] == '\0')
            return 0;

        if constexpr (SPathSemantics::SupportsVolumeNames)
        {
            // Drive letter, eg: "C:"
            if (IsAlpha(psz[0]) && psz[1] == ':')
                return 2;

            // UNC share, eg: "\\server\share"
            if (SPathSemantics::IsDirectorySeparator(psz[0]) &&
                SPathSemantics::IsDirectorySeparator(psz[1]))
            {
                // Skip the server and share names (two more separators, or the
                // end of the string)
                const char* p = psz + 2;
                int seps = 0;
                while (*p && seps < 2)
                {
                    if (SPathSemantics::IsDirectorySeparator(*p++))
                        seps++;
                }

                // Don't include the separator that terminates the share name
                if (p > psz + 2 && SPathSemantics::IsDirectorySeparator(p[-1]))
                    p--;

                return (int)(p - psz);
            }
        }

        return 0;
    }



    // Match a filename against a pattern. The pattern may contain '?' (any one
    // character) and '*' (any run of characters), and may hold several
    // alternatives separated by ';' (eg: "*.txt;*.md") - a match against any
    // one alternative is a match.
    template <typename T>
    static bool DoesMatchPattern(const T* filename, const T* pattern)
    {
        for (const T* p = pattern; ; )
        {
            if (MatchPatternSegment<T>(filename, p))
                return true;

            // Advance to the next ';'-separated alternative
            while (*p != '\0' && *p != ';')
                p++;
            if (*p == '\0')
                return false;
            p++;
        }
    }

    // Match a filename against a single pattern alternative, stopping at a ';'
    // or the end of the pattern string.
    template <typename T>
    static bool MatchPatternSegment(const T* f, const T* p)
    {
        while (true)
        {
            bool endOfPattern = (*p == '\0' || *p == ';');

            // End of both = match
            if (endOfPattern && *f == '\0')
                return true;

            // End of only one = no match
            if (endOfPattern || *f == '\0')
                return false;

            // Single character wildcard
            if (*p == '?')
            {
                p++;
                f++;
                continue;
            }

            // Multi-character wildcard
            if (*p == '*')
            {
                p++;
                if (*p == '\0' || *p == ';')
                    return true;
                while (*f != '\0')
                {
                    if (MatchPatternSegment<T>(f, p))
                        return true;
                    f++;
                }
                return false;
            }

            // Literal character
            if (SPathSemantics::FileSystemCase::Compare(*p, *f) != 0)
                return false;

            p++;
            f++;
        }
    }

private:
    static bool IsAlpha(char ch)
    {
        return (ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z');
    }
};

} // namespace
