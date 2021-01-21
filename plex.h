#ifndef __simplelib_plex_h__
#define __simplelib_plex_h__

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "semantics.h"

namespace SimpleLib
{
    // Simple block allocator used by CMap
    template <typename T>
    class CPlex
    {
    public:
        // Constructor
        CPlex(int iBlockSize = -1)
        {
            if (iBlockSize == -1)
            {
                iBlockSize = (256 - sizeof(BLOCK)) / sizeof(T);
            }

            if (iBlockSize < 4)
                iBlockSize = 4;

            m_iBlockSize = iBlockSize;

            m_pHead = nullptr;
            m_pFreeList = nullptr;
            m_iCount = 0;
        }

        // Destructor
        ~CPlex()
        {
            FreeAll();
        }

        // Allocate an item
        T* Alloc()
        {
            // If no free list, create a new block
            if (!m_pFreeList)
            {
                // Allocate a new block
                BLOCK* pNewBlock = (BLOCK*)malloc(sizeof(BLOCK) + m_iBlockSize * sizeof(T));

                // Add to list of blocks
                pNewBlock->m_pNext = m_pHead;
                m_pHead = pNewBlock;

                // Setup free list chain
                m_pFreeList = (FREEITEM*)&pNewBlock->m_bData[0];
                FREEITEM* p = m_pFreeList;
                for (int i = 0; i < m_iBlockSize - 1; i++)
                {
                    p->m_pNext = reinterpret_cast<FREEITEM*>(reinterpret_cast<char*>(p) + sizeof(T));
                    p = p->m_pNext;
                }

                // nullptr terminate the list
                p->m_pNext = nullptr;
            }

            // Remove top item from the free list
            T* p = (T*)m_pFreeList;
            m_pFreeList = m_pFreeList->m_pNext;

            // Update count
            m_iCount++;

            new ((void*)p) T;

            // Return pointer
            return (T*)p;
        }

        // Free an item
        void Free(T* p)
        {
            assert(m_iCount > 0);

            Destructor(p);

            if (m_iCount == 1)
            {
                // When freeing last item, free everything!
                FreeAll();
            }
            else
            {
                // Add to list of free items
                FREEITEM* pNewFreeItem = (FREEITEM*)p;
                pNewFreeItem->m_pNext = m_pFreeList;
                m_pFreeList = pNewFreeItem;

                // Update count
                m_iCount--;
            }
        }

        // Free all items
        void FreeAll()
        {
            // Release all blocks
            BLOCK* pBlock = m_pHead;
            while (pBlock)
            {
                // Save next
                BLOCK* pNext = pBlock->m_pNext;

                // Free it
                free(pBlock);

                // Move on
                pBlock = pNext;
            }

            // Reset state
            m_pHead = nullptr;
            m_pFreeList = nullptr;
            m_iCount = 0;
        }

        // Get the number of allocated items
        int GetCount() const
        {
            return m_iCount;
        }

        void SetBlockSize(int iNewBlockSize)
        {
            assert(m_pHead == nullptr && "SetBlockSize only support when plex is empty");
            m_iBlockSize = iNewBlockSize;
        }

    protected:


#ifdef _MSC_VER
#pragma warning(disable: 4200)
#endif
        struct BLOCK
        {
            BLOCK* m_pNext;
            char	m_bData[0];
        };
#ifdef _MSC_VER
#pragma warning(default: 4200)
#endif

        struct FREEITEM
        {
            FREEITEM* m_pNext;
        };

        BLOCK* m_pHead;
        FREEITEM* m_pFreeList;
        int			m_iCount;
        int			m_iBlockSize;
    };

} // namespace

#endif  // __simplelib_plex_h__