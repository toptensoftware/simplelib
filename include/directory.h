#ifndef __simplelib_directory_h__
#define __simplelib_directory_h__

#include "file.h"
#include "path.h"
#include "encoding.h"

namespace SimpleLib
{

enum IterateFlags
{
	Files = 0x01,
	Directories = 0x02,
	Recursive = 0x04,

	FilesAndDirectories = Files | Directories,
	AllFiles = Files | Recursive,
	AllDirectories = Directories | Recursive,
	All = Files | Directories | Recursive,
};

template <typename T>
class CDirectoryIterator
{
public:
	const T* Name;
	
	bool Next()
	{
	startAgain:
		// Quit if nothing left on stack
		if (_stack.IsEmpty())
			return false;

		// Get top of stack
		CState* pState = _stack.Tail();

		// Find next (unless already have it from FindFirst)
		if (pState->initial)
		{
			pState->initial = false;
		}
		else
		{
			if (!FindNextFileW(pState->hFind, &pState->fd))
			{
				_stack.Pop();
				goto startAgain;
			}
		}

		// Not interested in pseudo directories
		if (wcscmp(pState->fd.cFileName, L".") == 0 || 
			wcscmp(pState->fd.cFileName, L"..") == 0)
			goto startAgain;

		// Setup current entry
		_currentName = CPath::Join<T>(pState->prefix, Encode<T>(pState->fd.cFileName));
		Name = _currentName.sz();

		// Recurse
		if ((_flags & IterateFlags::Recursive) && (pState->fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0)
		{
			Add(CPath::Join<T>(pState->directory, Encode<T>(pState->fd.cFileName)));

			// Patch in the prefix name
			CState* pNewState = _stack.Tail();
			pNewState->prefix = _currentName;
		}

		// Ignore directory/files
		if (((pState->fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0) && (_flags & IterateFlags::Directories) == 0)
			goto startAgain;
		if (((pState->fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0) && (_flags & IterateFlags::Files) == 0)
			goto startAgain;

		if (!CDirectory::DoesMatchPattern<wchar_t>(pState->fd.cFileName, _pattern))
			goto startAgain;

		return true;
	}

private:
	CString<T> _currentName;
	IterateFlags _flags;
	CString<wchar_t> _pattern;

	int Init(const T* directory, const T* pattern, IterateFlags flags)
	{
		_flags = flags;
		_pattern = Encode<wchar_t>(pattern);
		return Add(directory);
	}

	int Add(const T* directory)
	{
		CState* pState = new CState();
		pState->hFind = FindFirstFileW(CPath::Join<wchar_t>(Encode<wchar_t>(directory), L"*").sz(), &pState->fd);
		pState->initial = true;
		pState->directory = directory;
		if (pState->hFind == INVALID_HANDLE_VALUE)
		{
			delete pState;
			return ENOENT;		// TODO
		}
		_stack.Push(pState);
		return 0;
	}

	struct CState
	{
		CState()
		{
			
		}
		~CState()
		{
			if (hFind != INVALID_HANDLE_VALUE)
				FindClose(hFind);
		}
		HANDLE hFind;
		WIN32_FIND_DATAW fd;
		bool initial;
		CString<T> prefix;
		CString<T> directory;
	};
	CVector<CState*, SOwnedPtr> _stack;

	friend class CDirectory;
};



// Abstract Stream Class
class CDirectory
{
public:

	// Check if a directory exists (and that it's a directory)
	template <typename T>
	static bool Exists(const T* name)
	{
		CFileInfo  info;
		return CFile::GetFileInfo(name, info) == 0 && info.IsDirectory;
	}

	// Create a directory path
	template <typename T>
	static int Create(const T* name)
	{
		// Quit if already exists
		if (Exists(name))
			return 0;

		// Create parent directory
		auto parent = CPath::GetDirectoryName<T>(name);
		if (!parent.IsEmpty())
		{
			int err = Create(parent.sz());
			if (err)
				return err;
		}

		// Create this directory
#ifdef _MSC_VER
		return _wmkdir(Encode<wchar_t>(name));
#else
		return mkdir(Encode<char>(name));
#endif
	}

	// Remove a directory path
	template <typename T>
	static int Delete(const T* name)
	{
#ifdef _MSC_VER
		return _wrmdir(Encode<wchar_t>(name));
#else
		return rmdir(Encode<char>(name));
#endif
	}

	#ifdef _WIN32
	typedef SCaseI DefaultFileSystemCase;
	#else
	typedef SCase DefaultFileSystemCase;
	#endif


	template <typename T, typename S = DefaultFileSystemCase>
	static bool DoesMatchPattern(const T* filename, const T* pattern)
	{
		const T* f = filename;
		const T* p = pattern;

		// Compare characters
		while (true)
		{
			// End of both strings = match!
			if ((*p=='\0' || *p==';') && *f=='\0')
				return true;

			// End of sub-pattern?
			if (*p==';' || *p=='\0' || *f=='\0')
			{
				return false;
			}

			// Single character wildcard
			if (*p=='?')
			{
				p++;
				f++;
				continue;
			}

			// Multi-character wildcard
			if (*p=='*')
			{
				p++;
				if (*p == '\0')
					return true;
				while (*f!='\0')
				{
					if (DoesMatchPattern<T,S>(f, p))
						return true;
					f++;
				}
				return false;
			}

			// Same character?
			if (S::Compare(*p, *f) != 0)
				return false;

			// Next
			p++;
			f++;
		}
	}

	template <typename T, typename S = DefaultFileSystemCase>
	static int Iterate(const T* directory, const T* pattern, IterateFlags flags, CDirectoryIterator<T>& iter)
	{
		return iter.Init(directory, pattern, flags);
	}
};


}	// namespace

#endif // __simplelib_directory_h__