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

class CDirectoryIterator
{
public:
	const char* Name;
	
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
		_currentName = CPath::Join(pState->prefix, Encode<char>(pState->fd.cFileName));
		Name = _currentName.sz();

		// Recurse
		if ((_flags & IterateFlags::Recursive) && (pState->fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0)
		{
			Add(CPath::Join(pState->directory, Encode<char>(pState->fd.cFileName)));

			// Patch in the prefix name
			CState* pNewState = _stack.Tail();
			pNewState->prefix = _currentName;
		}

		// Ignore directory/files
		if (((pState->fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0) && (_flags & IterateFlags::Directories) == 0)
			goto startAgain;
		if (((pState->fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0) && (_flags & IterateFlags::Files) == 0)
			goto startAgain;

		if (!CPath::DoesMatchPattern<wchar_t>(pState->fd.cFileName, _pattern))
			goto startAgain;

		return true;
	}

private:
	CString _currentName;
	IterateFlags _flags;
	CCoreString<wchar_t> _pattern;

	int Init(const char* directory, const char* pattern, IterateFlags flags)
	{
		_flags = flags;
		_pattern = Encode<wchar_t>(pattern);
		return Add(directory);
	}

	int Add(const char* directory)
	{
		CState* pState = new CState();
		pState->hFind = FindFirstFileW(Encode<wchar_t>(CPath::Join(directory, "*").sz()), &pState->fd);
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
		CString prefix;
		CString directory;
	};
	CVector<CState*, SOwnedPtr> _stack;

	friend class CDirectory;
};



// Abstract Stream Class
class CDirectory
{
public:

	// Check if a directory exists (and that it's a directory)
	static bool Exists(const char* name)
	{
		CFileInfo  info;
		return CFile::GetFileInfo(name, info) == 0 && info.IsDirectory;
	}

	// Create a directory path
	static int Create(const char* name)
	{
		// Quit if already exists
		if (Exists(name))
			return 0;

		// Create parent directory
		auto parent = CPath::GetDirectoryName(name);
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
		return mkdir(name);
#endif
	}

	// Remove a directory path
	static int Delete(const char* name)
	{
#ifdef _MSC_VER
		return _wrmdir(Encode<wchar_t>(name));
#else
		return rmdir(name);
#endif
	}


	template <typename S = CPath::DefaultFileSystemCase>
	static int Iterate(const char* directory, const char* pattern, IterateFlags flags, CDirectoryIterator& iter)
	{
		return iter.Init(directory, pattern, flags);
	}
};


}	// namespace

#endif // __simplelib_directory_h__