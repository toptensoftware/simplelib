#ifndef __simplelib_stream_h__
#define __simplelib_stream_h__

#include <stdint.h>
#include <stdio.h>
#include <assert.h>
#include <errno.h>

#ifdef _MSC_VER
#include <windows.h>
#include <io.h>
#define __off64_t int64_t
#endif

#ifdef __GNUC__
#include <unistd.h>
#endif

namespace SimpleLib
{

// Abstract Stream Class
class CStream
{
public:
// Construction
			CStream() {}
	virtual ~CStream() {}

// Abstract operations
	virtual void Close() = 0;
	virtual bool IsOpen() = 0;
	virtual int Read(void* pv, size_t cb, size_t* pcb = nullptr) = 0;
	virtual int Write(const void* pv, size_t cb) = 0;
	virtual int Seek(__off64_t offset, int origin = SEEK_SET) = 0;
	virtual __off64_t Tell() = 0;
	virtual __off64_t GetLength() = 0;
	virtual int Truncate() = 0;
	virtual bool IsEof() = 0;

	// Typed Read/Write
	template <typename T>
	int Read(T& t) { return Read(&t, sizeof(T), nullptr); }
	template <typename T>
	int Write(const T& t) { return Write(&t, sizeof(T)); }

	// Copy everything from one stream to another
	static int Copy(CStream& dest, CStream& src)
	{
		char buf[4096];
		while (true)
		{
			// Read
			size_t cb;
			int err = src.Read(buf, sizeof(buf), &cb);
			if (err != 0 && err != EOF)
				return err;

			// Write
			if (cb)
			{
				err = dest.Write(buf, cb);
				if (err)
					return err;
			}

			// Quit if finished
			if (cb < sizeof(buf))
				return 0;
		}
	}
};

template<typename T>
class CStreamStringWriter : public IStringWriter<T>
{
public:
	CStreamStringWriter(CStream& stream) : 
		_stream(stream)
	{
		_err = 0;
	}

	int GetError()
	{
		return _err;
	}

	virtual void Write(T ch)
	{
		if (_err)
			return;
		_err = _stream.Write(ch);
	}

	 CStream& _stream;
	 int _err;
};

// Stream class for reading and writing files
class CFileStream : public CStream
{
public:
	CFileStream() 
	{
		m_pFile = nullptr;
	}

	virtual ~CFileStream()
	{
		Close();
	}

	// Open a file with optional mode (defaults to read)
	template <typename T>
	int Open(const T* filename, const char* mode = "rb")
	{
		assert(m_pFile == nullptr);

		m_pFile = fopen<T>(filename, mode);
		return m_pFile == nullptr ? errno : 0;
	}

	// Create a new file
	template <typename T>
	int Create(const T* filename)
	{
		return Open(filename, "wb+");
	}

	// Helper to fopen a file with any charset filename
	template <typename T>
	static FILE* fopen(const T* filename, const char* mode)
	{
#ifdef _WIN32
		return _wfopen((wchar_t*)Encode<char16_t>(filename).sz(), (wchar_t*)Encode<char16_t>(mode).sz());
#else
		return ::fopen(Encode<char>(filename).sz(), mode);
#endif
	};

	// Close file if open
	virtual void Close() override
	{
		if (m_pFile != nullptr)
		{
			fclose(m_pFile);
			m_pFile = nullptr;
		}
	}

	// IsOpen
	virtual bool IsOpen() override
	{
		return m_pFile != nullptr;
	}

	// Read
	virtual int Read(void* pv, size_t cb, size_t* pcb = nullptr) override
	{
		assert(m_pFile != nullptr);
		assert(pv != nullptr);

		// Read
		size_t cbRead = fread(pv, 1, cb, m_pFile);

		// Return bytes read if asked for
		if (pcb != nullptr)
			*pcb = cbRead;

		// Handle didn't read
		if (cbRead != cb)
		{
			if (IsEof())
				return EOF;
			return errno;
		}

		return 0;
	}

	// Write
	virtual int Write(const void* pv, size_t cb) override
	{
		if (fwrite(pv, 1, cb, m_pFile) != cb)
			return errno;
		return 0;
	}

	// Seek
	virtual int Seek(__off64_t offset, int origin = SEEK_SET) override
	{
#ifdef _MSC_VER
		return _fseeki64(m_pFile, offset, origin);
#else
		return fseeko64(m_pFile, offset, origin);
#endif
	}

	// Tell
	virtual __off64_t Tell() override
	{
#ifdef _MSC_VER
		return _ftelli64(m_pFile);
#else
		return ftello64(m_pFile);
#endif
	}

	// Get length
	virtual __off64_t GetLength() override
	{
		__off64_t save = Tell();
		Seek(0, SEEK_END);
		__off64_t length = Tell();
		Seek(save, SEEK_SET);
		return length;
	}

	// Truncate
	virtual int Truncate() override
	{
#ifdef _WIN32
		::SetEndOfFile((HANDLE)_get_osfhandle(fileno(m_pFile)));
		return 0;		// don't really care
#else
		return ftruncate(fileno(m_pFile), Tell());
#endif
	}

	// EOF?
	virtual bool IsEof() override
	{
		return feof(m_pFile);
	}


	FILE* m_pFile;
};

// Stream class for reading and writing blocks of memory
class CMemoryStream : public CStream
{
public:
	// Constructor
	CMemoryStream() 
	{
		m_p = nullptr;
		m_length = 0;
		m_pos = 0;
		m_owned = false;
		m_eof = false;
	}

	// Destructor
	virtual ~CMemoryStream()
	{
		Close();
	}

	// Create a new read/writable memory stream
	int Create()
	{
		assert(m_p == nullptr);

		m_allocated = 4096;
		m_owned = true;
		m_pos = 0;
		m_length = 0;
		m_eof = false;
		m_p = malloc((size_t)m_allocated);
		if (m_p == nullptr)
		{
			Close();
			return 1;
		}

		return 0;
	}

	// Open an existing memory stream.  If takeOwnership is
	// true, the stream is writable and the memory pointed
	// to by p must have been allocated with malloc.
	int Open(void* p, size_t length, bool takeOwnership)
	{
		assert(m_p == nullptr);

		m_allocated = length;
		m_owned = takeOwnership;
		m_pos = 0;
		m_length = length;
		m_eof = false;
		m_p = p;
		return 0;
	}

	// Create a new read/write stream initialized
	// with a copy of supplied data
	int InitWith(void* p, __off64_t length)
	{
		assert(m_p == nullptr);

		m_allocated = length;
		m_owned = true;
		m_pos = 0;
		m_length = length;
		m_eof = false;
		m_p = malloc((size_t)length);
		if (p)
			memcpy(m_p, p, (size_t)length);
		return 0;
	}

	// Detach the memory buffer, return it and close this stream
	void* CloseAndDetach(size_t* pcb)
	{
		assert(m_p != nullptr);

		if (pcb)
			*pcb=m_length;

		void* p = m_p;
		m_owned = false;
		Close();
		return p;
	}

	// Get a pointer to the underlying memory buffer
	void* GetBuffer()
	{
		assert(m_p != nullptr);
		return m_p;
	}

	// Get the current capacity of this memory stream
	size_t GetCapacity()
	{
		assert(m_p != nullptr);
		return m_allocated;
	}

	// Close file if open
	virtual void Close() override
	{
		if (m_owned && m_p)
			free(m_p);
		m_p = nullptr;
		m_length = 0;
		m_pos = 0;
		m_owned = false;
		m_eof = false;
	}

	// IsOpen
	virtual bool IsOpen() override
	{
		return m_p != nullptr;
	}

	// Read
	virtual int Read(void* pv, size_t cb, size_t* pcb) override
	{
		assert(m_p != nullptr);

		// Check for EOF
		if (m_pos + cb > m_length)
		{
			cb = m_length - m_pos;
			m_eof = true;
 		}

		// Read em
		memcpy(pv, (char*)m_p + m_pos, cb);

		// Update current position
		m_pos += cb;

		if (pcb)
			*pcb = cb;

		return m_pos <= m_length ? 0 : EOF;
	}

	// Write
	virtual int Write(const void* pv, size_t cb) override
	{
		assert(m_p != nullptr);

		// Writable?
		if (!m_owned)
		{
			return EPERM;
		}

		m_eof = false;

		// Grow buffer
		if (m_pos + cb > m_allocated)
		{
			// Double the buffer size until it's big enough
			size_t newSize = m_allocated * 2;
			while (m_pos + cb > newSize)
				newSize *= 2;

			// Reallocate memory
			void* p = realloc(m_p, newSize);
			if (p == nullptr)
			{
				return ENOMEM;
			}
			m_p = p;
		}

		// Write it
		memcpy((char*)m_p + m_pos, pv, cb);

		// Update position
		m_pos += cb;

		// Length extended?
		if (m_pos > m_length)
			m_length = m_pos;

		return 0;
	}

	// Seek
	virtual int Seek(__off64_t offset, int origin = SEEK_SET) override
	{
		assert(m_p != nullptr);

		m_eof = false;

		switch (origin)
		{
			case SEEK_SET:
				m_pos = offset;
				break;

			case SEEK_CUR:
				m_pos += offset;
				break;

			case SEEK_END:
				m_pos = m_length - offset;
				break;
		}
		return 0;
	}

	// Tell
	virtual __off64_t Tell() override
	{
		assert(m_p != nullptr);
		return m_pos;
	}

	// Get length
	virtual __off64_t GetLength() override
	{	
		assert(m_p != nullptr);
		return m_length;
	}

	// Truncate
	virtual int Truncate() override
	{
		int err = Write(nullptr, 0);
		if (err)
			return err;
		m_length = m_pos;
		m_eof = false;
		return 0;
	}

	// EOF?
	virtual bool IsEof() override
	{
		return m_eof;
	}

protected:
	void* m_p;
	size_t m_length;
	size_t m_pos;
	size_t m_allocated;
	bool m_owned;
	bool m_eof;
};

}	// namespace

#endif // __simplelib_stream_h__