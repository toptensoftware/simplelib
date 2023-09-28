#pragma once

#include "stream.h"
#include "string.h"
#include "stringbuilder.h"
#include "encoding.h"

#ifdef __GNUC__
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <fcntl.h>
#endif

namespace SimpleLib
{
	class FileInfo
	{
	public:
		bool IsFile;
		bool IsDirectory;
		__off64_t Size;
		int64_t AccessTime;
		int64_t ModifyTime;
		int64_t CreateTime;	
	};

	// Abstract Stream Class
	class File
	{
	public:

		// Reads a UTF-8 encoding text file into a CString
		static int ReadAllText(const char* filename, CString& text)
		{
			// Clear text
			text.Clear();

			// Open file
			FileStream file;
			int err = file.Open(filename);
			if (err)
				return err;

			// Setup encoder
			CStringBuilder out;

			// Read it
			char buf[4096];
			while (true)
			{
				// Read
				size_t cb;
				int err = file.Read(buf, sizeof(buf), &cb);
				if (err != 0 && err != EOF)
					return err;

				// Append
				out.Append(buf, cb);

				// Quit if finished
				if (cb < sizeof(buf))
					break;
			}

			// Return result
			text = CString(out);
			return 0;
		}

		// Writes a UTF-8 encoded text file
		static int WriteAllText(const char* filename, const char* text)
		{
			// Create the file
			FileStream file;
			int err = file.Create(filename);
			if (err)
				return err;

			int length = SChar<char>::Length(text);
			return file.Write(text, length);
		}

		// Delete a file
		static int Delete(const char* filename)
		{
	#ifdef _MSC_VER
			return _wunlink(Encode<wchar_t>(filename));
	#else
			return unlink(Encode<char>(filename));
	#endif
		}

		// Get info about a file
		static int GetFileInfo(const char* filename, FileInfo& info)
		{
	#ifdef _MSC_VER
			struct __stat64 s;
			int err = _wstat64(Encode<wchar_t>(filename), &s);
			if (!err)
			{
				info.IsDirectory = (s.st_mode & _S_IFDIR) != 0;
				info.IsFile = (s.st_mode & _S_IFREG) != 0;
				info.Size = s.st_size;
				info.AccessTime = s.st_atime;
				info.ModifyTime = s.st_mtime;
				info.CreateTime = s.st_ctime;
			}
	#else
			struct stat64 s;
			int err = stat64(filename, &s);
			if (!err)
			{
				info.IsDirectory = (s.st_mode & S_IFDIR) != 0;
				info.IsFile = (s.st_mode & S_IFREG) != 0;
				info.Size = s.st_size;
				info.AccessTime = s.st_atime;
				info.ModifyTime = s.st_mtime;
				info.CreateTime = s.st_ctime;
			}
	#endif
			return err;
		}

		// Check if a file exists
		static bool Exists(const char* filename)
		{
			FileInfo  info;
			return GetFileInfo(filename, info) == 0 && info.IsFile;
		}

		// Copy a file
		static int Copy(const char* source, const char* dest, bool overwrite)
		{
	#ifdef _MSC_VER
			if (CopyFile(Encode<wchar_t>(source), Encode<wchar_t>(dest), !overwrite))
				return 0;
			if (GetLastError() == ERROR_FILE_EXISTS)
				return EEXIST;
			else
				return EPERM;
	#else

		// Open source file
		int fd_in = open(source, O_RDONLY);
		if (fd_in == -1)
			return errno;
			
		// Stat it
		struct stat64 stat;
		if (fstat64(fd_in, &stat) == -1) 
		{
			int err = errno;
			close(fd_in);
			return err;
		}

		// Create dest file
		int fd_out = open(dest, O_CREAT | O_WRONLY | (overwrite ? 0 : O_EXCL), 0644);
		if (fd_out == -1) 
		{
			int err = errno;
			close(fd_in);
			return err;
		}

		// Copy
		int64_t len = stat.st_size;
		while (len > 0)
		{
			ssize_t copied = copy_file_range(fd_in, NULL, fd_out, NULL, len, 0);
			if (copied < 0)
			{
				int err = errno;
				close(fd_in);
				close(fd_out);
				Delete(dest);
				return err;
			}

			len -= copied;
		}

		// Copy file times too
		struct timeval t[2];
		TIMESPEC_TO_TIMEVAL(&t[0], &stat.st_atim);
		TIMESPEC_TO_TIMEVAL(&t[1], &stat.st_mtim);
		futimes(fd_out, t);

		// Done!
		close(fd_in);
		close(fd_out);
		return 0;
	#endif
		}

		// Move (aka Rename) a file
		static int Move(const char* source, const char* dest)
		{
	#ifdef _MSC_VER
		return _wrename(Encode<wchar_t>(source), Encode<wchar_t>(dest));
	#else
		return rename(source, dest);
	#endif
		}

	};

}
