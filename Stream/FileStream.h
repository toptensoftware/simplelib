#pragma once

#include <io.h>

#include "Stream.h"
#include "../Core/Encoding.h"

namespace SimpleLib
{


class FileStream : public Stream
{
public:

	FileStream(FILE* file = nullptr)
	{
		m_file = nullptr;
	}

	virtual ~FileStream()
	{
		Close();
	}

	// Opens a file in this instance
	int Open(const char* pszFileName, const char* pszMode, int shFlag = 0)
	{
		assert(m_file == nullptr);

#ifdef _MSC_VER
		if (shFlag == 0)
			shFlag = _SH_DENYWR;
#endif

		m_file = _wfsopen(Encode<wchar_t>(pszFileName), Encode<wchar_t>(pszMode), shFlag);
		if (!m_file)
		{
			return errno;
		}

		return 0;
	}

	// Create an instance and open the underlying file
	static FileStream* Create(const char* pszFileName, const char* pszMode, int shFlag = 0)
	{
#ifdef _MSC_VER
		if (shFlag == 0)
			shFlag = _SH_DENYWR;
#endif
		FILE* file = _wfsopen(Encode<wchar_t>(pszFileName), Encode<wchar_t>(pszMode), shFlag);
		if (file)
			return new FileStream(file);
		else
			return nullptr;
	}

	void Close()
	{
		if (m_file != nullptr)
		{
			fclose(m_file);
			m_file = nullptr;
		}
	}

	virtual bool IsOpen() override
	{
		return m_file != nullptr;
	}


	virtual int Read(void* pv, uint32_t cb, uint32_t* pcb = nullptr) override
	{
		assert(m_file != nullptr);
		errno = 0;
		uint32_t cbRead = (uint32_t)fread(pv, 1, cb, m_file);
		if (errno != 0)
			return errno;

		if (pcb)
		{
			*pcb = cbRead;
		}
		return 0;
	}

	virtual int Write(const void* pv, uint32_t cb, uint32_t* pcb = nullptr) override
	{
		assert(m_file != nullptr);
		errno = 0;
		uint32_t cbWrite = (uint32_t)fwrite(pv, 1, cb, m_file);
		if (errno != 0)
			return errno;
		if (pcb)
		{
			*pcb = cbWrite;
		}
		return 0;
	}

	virtual int Seek(int64_t offset, int origin = SEEK_SET) override
	{
		assert(m_file != nullptr);
		return _fseeki64(m_file, offset, origin);
	}

	virtual int64_t Tell() override
	{
		assert(m_file != nullptr);
		return _ftelli64(m_file);
	}

	virtual int64_t Length() override
	{
		assert(m_file != nullptr);
		int64_t save = Tell();
		Seek(0, SEEK_END);
		int64_t length = Tell();
		Seek(save, SEEK_SET);
		return length;
	}

	virtual int SetLength() override
	{
		assert(m_file != nullptr);
		return _chsize_s(_fileno(m_file), Tell());
	}

	virtual int Flush() override
	{
		assert(m_file != nullptr);
		return fflush(m_file);
	}

private:
	FILE* m_file;
};



}
