#pragma once

#include <malloc.h>
#include "Stream.h"

#ifdef _WIN32
#include <windows.h>
#endif

namespace SimpleLib
{


class MemoryStream : public Stream
{
public:

    MemoryStream()
    {
        m_p = nullptr;
        m_length = 0;
        m_pos = 0;
        m_owned = false;
    #ifdef _WIN32
        m_hResMem = nullptr;
    #endif
    }

    virtual ~MemoryStream()
    {
        Close();
    }

    int Create()
    {
        assert(m_p==nullptr);

        m_allocated = 4096;
        m_owned = true;
        m_pos = 0;
        m_length = 0;
        m_p = malloc((size_t)m_allocated);
        if (m_p==nullptr)
        {
            Close();
            return 1;
        }

        return 0;
    }

    int Open(void* p, int64_t length, bool takeOwnership)
    {
        assert(m_p == nullptr);

        Close();

        m_allocated = length;
        m_owned = takeOwnership;
        m_pos = 0;
        m_length = length;
        m_p = p;
        return 0;
    }

    int Init(void* p, int64_t length)
    {
        assert(m_p == nullptr);

        Close();

        m_allocated = length;
        m_owned = true;
        m_pos = 0;
        m_length = length;
        m_p = malloc((size_t)length);
        if (p)
            memcpy(m_p, p, (size_t)length);
        return 0;
    }

    #ifdef _WIN32
    int OpenResource(HMODULE hModule, LPCTSTR pszName, LPCTSTR pszType)
    {
        HRSRC hRes = FindResource(hModule, pszName, pszType);
        if (hRes==nullptr)
            return ENOENT;

        HGLOBAL hResMem = LoadResource(hModule, hRes);
        if (hResMem==nullptr)
            return ENOENT;

        void* pRes = LockResource(hResMem);
        m_hResMem = hResMem;

        return Open(pRes, SizeofResource(hModule, hRes), false);
    }
    #endif

    void Close()
    {
        if (m_owned && m_p)
        {
            free(m_p);
        }
        m_p = nullptr;
        m_length = 0;
        m_pos = 0;
        m_owned = false;
    #ifdef _WIN32
        if (m_hResMem)
            UnlockResource(m_hResMem);
        m_hResMem = nullptr;
    #endif
    }

    void* GetBuffer()
    {
        assert(m_p!=nullptr);
        return m_p;
    }

    void MoveFrom(MemoryStream& other)
    {
        Close();

        int64_t len;
        void* pMem = other.CloseAndDetach(&len);
        Open(pMem, len, true);
    }


    bool IsEqual(MemoryStream& other)
    {
        if (m_length!=other.m_length)
            return false;
        if (m_length==0)
            return true;

        return memcmp(m_p, other.m_p, (size_t)m_length)==0;
    }

    void* CloseAndDetach(int64_t* pcb)
    {
        assert(m_p!=nullptr);

        if (pcb)
            *pcb=m_length;

        void* p = m_p;
        m_owned = false;
        Close();
        return p;
    }


    virtual int Read(void* pv, uint32_t cb, uint32_t* pcb = nullptr) override
    {
        assert(m_p!=nullptr);
        uint32_t cbRequested = cb;

        // Check for EOF
        if (m_pos + cb >= m_length)
        {
            cb = (uint32_t)(m_length - m_pos);
        }

        // Read em
        memcpy(pv, (char*)m_p + m_pos, cb);

        // Update current position
        m_pos += cb;


        // Return number of bytes actually read
        if (pcb)
        {
            *pcb = cb;
        }

        return 0;
    }

    virtual int Write(const void* pv, uint32_t cb, uint32_t* pcb = nullptr) override
    {
        assert(m_p!=nullptr);

        // Writable?
        if (!m_owned)
        {
            if (pcb)
                *pcb = 0;
            return 1;
        }

        // Grow buffer
        if (m_pos + cb > m_allocated)
        {
            // Double the buffer size until it's big enough
            int64_t newSize = m_allocated * 2;
            while (m_pos + cb > newSize)
                newSize *= 2;

            // Reallocate memory
            void* p = realloc(m_p, (size_t)newSize);
            if (p==nullptr)
            {
                if (pcb)
                    *pcb = 0;
                return ENOMEM;
            }
            m_p = p;
        }

        // Write it
        memcpy((char*)m_p + m_pos, pv, cb);

        // Update position
        m_pos += cb;

        // Update length
        if (m_pos > m_length)
            m_length = m_pos;

        if (pcb)
            *pcb = cb;
        return 0;
    }

    virtual int Seek(int64_t offset, int origin) override
    {
        assert(m_p!=nullptr);

        switch (origin)
        {
            case SEEK_SET:
                m_pos = offset;
                break;

            case SEEK_CUR:
                m_pos += offset;
                break;

            case SEEK_END:
                m_pos = m_length + offset;
                break;
        }
        return 0;
    }

    virtual int64_t Tell() override
    {
        assert(m_p!=nullptr);
        return m_pos;
    }

    virtual int64_t Length() override
    {
        assert(m_p!=nullptr);
        return m_length;
    }

    virtual int SetLength() override
    {
        RIFE(Write(nullptr, 0, nullptr));
        m_length = m_pos;
        return 0;
    }


	int Save(Stream& stream)
	{
		stream.WriteInt32((int32_t)Length());
		return stream.Write(GetBuffer(), (int32_t)Length());
	}

	int Load(Stream& stream)
	{
		int length = stream.ReadInt32();
		void* pMem = malloc(length);
		RIFE(stream.Read(pMem, length));
		return Open(pMem, length, true);
	}

private:
	void* m_p;
	int64_t m_length;
	int64_t m_pos;
	int64_t m_allocated;
	bool m_owned;
#ifdef _WIN32
	void* m_hResMem;
#endif
};



}