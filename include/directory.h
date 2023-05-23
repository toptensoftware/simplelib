#ifndef __simplelib_directory_h__
#define __simplelib_directory_h__

#include "file.h"
#include "path.h"
#include "encoding.h"
#include "sharedptr.h"

#ifndef _WIN32
#include <sys/types.h>
#include <dirent.h>
#endif

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

template <typename SPathSemantics>
class Directory;


template <typename SPathSemantics = SPathSemanticsAuto>
class DirectoryIterator
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
		SharedPtr<CState> pState = _stack.Tail();
		if (!pState->Next())
		{
			_stack.Pop();
			goto startAgain;
		}

		// Setup current entry
		Name = pState->_currentName.sz();

		// Recurse
		if ((_flags & IterateFlags::Recursive) && (pState->_currentItemFlags & IterateFlags::Directories))
		{
			Add(Path<SPathSemantics>::Join(_baseDirectory, pState->_currentName), pState->_currentName);
		}

		// Ignore directory/files
		if ((pState->_currentItemFlags & _flags) == 0)
			goto startAgain;

		if (!Path<SPathSemantics>::template DoesMatchPattern<char>(pState->_fileName, _pattern))
			goto startAgain;

		return true;
	}

private:
	IterateFlags _flags;
	CString _baseDirectory;
	CString _pattern;

	int Init(const char* directory, const char* pattern, IterateFlags flags)
	{
		_flags = flags;
		_pattern = pattern;
		_baseDirectory = directory;
		return Add(directory, nullptr);
	}

	int Add(const char* directory, const char* prefix)
	{
		CState* pState = new CState();
		int err = pState->Init(directory, prefix);
		if (err)
		{
			delete pState;
			return err;
		}
		_stack.Push(pState);
		return 0;
	}

#ifdef _WIN32
	struct CState
	{
		CState()
		{
			
		}
		~CState()
		{
			if (_hFind != INVALID_HANDLE_VALUE)
				FindClose(_hFind);
		}
		int Init(const char* directory, const char* prefix)
		{
			_hFind = FindFirstFileW(Encode<wchar_t>(Path<SPathSemantics>::Join(directory, "*").sz()), &_fd);
			_initial = true;
			_directory = directory;
			_prefix = prefix;
			if (_hFind == INVALID_HANDLE_VALUE)
			{
				return ENOENT;		// TODO
			}
			return 0;
		}
		bool Next()
		{
			startAgain:
			// Find next (unless already have it from FindFirst)
			if (_initial)
			{
				_initial = false;
			}
			else
			{
				if (!FindNextFileW(_hFind, &_fd))
					return false;
			}

			// Not interested in pseudo directories
			if (wcscmp(_fd.cFileName, L".") == 0 || 
				wcscmp(_fd.cFileName, L"..") == 0)
				goto startAgain;

			if (_fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
				_currentItemFlags = IterateFlags::Directories;
			else
				_currentItemFlags = IterateFlags::Files;

			_fileName = Encode<char>(_fd.cFileName);
			_currentName = Path<SPathSemantics>::Join(_prefix, _fileName);
			return true;
		}

		CString _fileName;
		CString _currentName;
		IterateFlags _currentItemFlags;
		HANDLE _hFind;
		WIN32_FIND_DATAW _fd;
		bool _initial;
		CString _prefix;
		CString _directory;
	};
#else
	struct CState
	{
		CState()
		{
			_dir = 0;
		}
		~CState()
		{
			if (_dir != nullptr)
				closedir(_dir);
		}
		int Init(const char* directory, const char* prefix)
		{
			_directory = directory;
			_prefix = prefix;
			_dir = opendir(directory);
			if (_dir == nullptr)
				return errno;
			return 0;
		}
		bool Next()
		{
			startAgain:
			struct dirent* pde = readdir(_dir);
			if (pde == nullptr)
				return false;

			if (strcmp(pde->d_name, ".") == 0 || 
				strcmp(pde->d_name, "..") == 0)
				goto startAgain;

			_fileName = pde->d_name;
			_currentName = CPath::Join(_prefix, _fileName);

			if (pde->d_type & DT_DIR)
				_currentItemFlags = IterateFlags::Directories;
			else if (pde->d_type & DT_REG)
				_currentItemFlags = IterateFlags::Files;
			else
				goto startAgain;

			return true;
		}

		DIR* _dir;
		CString _fileName;
		CString _currentName;
		IterateFlags _currentItemFlags;
		CString _prefix;
		CString _directory;
	};
#endif

	List<SharedPtr<CState>> _stack;

	friend class Directory<SPathSemantics>;
};



// Abstract Stream Class
template <typename SPathSemantics = SPathSemanticsAuto>
class Directory
{
public:

	// Check if a directory exists (and that it's a directory)
	static bool Exists(const char* name)
	{
		FileInfo  info;
		return File::GetFileInfo(name, info) == 0 && info.IsDirectory;
	}

	// Create a directory path
	static int Create(const char* name)
	{
		// Quit if already exists
		if (Exists(name))
			return 0;

		// Create parent directory
		auto parent = Path<SPathSemantics>::GetDirectoryName(name);
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
		return mkdir(name, 0644);
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


	static int Iterate(const char* directory, const char* pattern, IterateFlags flags, DirectoryIterator<SPathSemantics>& iter)
	{
		return iter.Init(directory, pattern, flags);
	}
};


}	// namespace

#endif // __simplelib_directory_h__