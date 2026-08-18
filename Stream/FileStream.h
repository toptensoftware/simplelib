#pragma once

#include "Stream.h"
#include "../Core/Encoding.h"
#include "../Platform/Platform.h"

namespace SimpleLib
{


class FileStream : public Stream
{
public:

	FileStream()
	{
		Platform::Init(m_file);
	}

	virtual ~FileStream()
	{
		Platform::Close(m_file);
	}

	// Opens a file in this instance
	int Open(const char* pszFileName, const char* pszMode)
	{
		return Platform::Open(m_file, pszFileName, pszMode);
	}

	void Close()
	{
		Platform::Close(m_file);
	}

	virtual bool IsOpen() override
	{
		return Platform::IsOpen(m_file);
	}

	virtual int Read(void* pv, size_t cb, size_t* pcb = nullptr) override
	{
		assert(Platform::IsOpen(m_file));
		return Platform::Read(m_file, pv, cb, pcb);
	}

	virtual int Write(const void* pv, size_t cb, size_t* pcb = nullptr) override
	{
		assert(Platform::IsOpen(m_file));
		return Platform::Write(m_file, pv, cb, pcb);
	}

	virtual int Seek(int64_t offset, int origin = SEEK_SET) override
	{
		assert(Platform::IsOpen(m_file));
		return Platform::Seek(m_file, offset, origin);
	}

	virtual int64_t Tell() override
	{
		assert(Platform::IsOpen(m_file));
		return Platform::Tell(m_file);
	}

	virtual int64_t GetLength() override
	{
		assert(Platform::IsOpen(m_file));
		int64_t save = Tell();
		Seek(0, SEEK_END);
		int64_t length = Tell();
		Seek(save, SEEK_SET);
		return length;
	}

	virtual int Truncate() override
	{
		assert(Platform::IsOpen(m_file));
		return Platform::Truncate(m_file);
	}

	virtual int Flush() override
	{
		assert(Platform::IsOpen(m_file));
		return Platform::Flush(m_file);
	}

private:
	Platform::TFile m_file;
};



}
