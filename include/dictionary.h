#pragma once

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "placed_constructor.h"
#include "compare.h"
#include "plex.h"

/*

Map class implements a map with semantics support.  Implemented as a red-black tree
with linked-list between values for fast iteration.

Supports:
	* Pseudo random access - operator[](int iIndex)
	* Insert/Delete during iteration
	* Semanatics

eg:

	// map of ints to int
	Dictionary<int, int>			mapInts;

	// map of ints to object
	Dictionary<int, CMyObject>	mapObjects;

	// map of ints to object ptrs
	Dictionary<int, CMyObject*>	mapPtrObjects;

	// a map of ints to pointers where pointers deleted on removal
	Dictionary<int, SharedPtr<CMyObject>> mapPtrObjects;

	// a map of strings to integers, with case insensitivity on the keys
	Dictionary< CString<char>, int, SCaseI>	map;
	map.Add("Apples", 10);
	map.Add("Pears", 20);
	map.Add("Bananas", 30);

	// Iterating the above map
	for (int i=0; i<map.GetSize(); i++)
	{
		printf("%s - %i\n", map[i].Key, map[i].Value);
	}
*/

namespace SimpleLib
{
    template <typename TKey, typename TValue, typename TKeyCompare = SDefaultCompare>
    class Dictionary
    {
    public: 
        // Constructor
        Dictionary() :
            m_pRoot(&m_Leaf),
            m_iSize(0)
        {
            m_Leaf.m_pParent = nullptr;
            m_Leaf.m_pLeft= &m_Leaf;
            m_Leaf.m_pRight = &m_Leaf;
            m_Leaf.m_bRed = false;
            m_iIterPos=-1;
            m_pIterNode=nullptr;
        }

        // Destructor
        virtual ~Dictionary()
        {
            FreeNode(m_pRoot);
        }

    // Type used as return value from operator[]
        class KeyPair
        {
        public:
            KeyPair(const TKey& Key, TValue& Value) :
                Key(Key),
                Value(Value)
            {
            }
            KeyPair(const KeyPair& Other) :
                Key(Other.Key),
                Value(Other.Value)
            {
            }

            const TKey&	Key;
            TValue&	Value;

    #ifdef _MSC_VER
        private:
            // "Occassionally" MSVC compiler gives warning about inability
            // to generate assignment operator that it never even uses.
            // This seems to fix warning and will give link error if actually needed
            KeyPair& operator=(const KeyPair& Other);
    #endif
        };


        // Get number of elements in map
        int GetCount() const 
        {
            return m_iSize;
        }

        // Is the map empty?
        bool IsEmpty() const
        {
            return m_iSize==0;
        }

        // Index based iteration
        KeyPair GetAt(int iIndex) const
        {
            assert(iIndex>=0 && iIndex<m_iSize);
        #ifdef _DEBUG_CHECKS
            CheckAll();
        #endif

            if (iIndex==0)
            {
                m_iIterPos=0;
                m_pIterNode=m_pFirst;
                return KeyPair(m_pIterNode->m_KeyPair.m_Key, m_pIterNode->m_KeyPair.m_Value);
            }

            if (iIndex==m_iSize-1)
            {
                m_iIterPos=m_iSize-1;
                m_pIterNode=m_pLast;
                return KeyPair(m_pIterNode->m_KeyPair.m_Key, m_pIterNode->m_KeyPair.m_Value);
            }

            if (iIndex==m_iIterPos)
            {
                return KeyPair(m_pIterNode->m_KeyPair.m_Key, m_pIterNode->m_KeyPair.m_Value);
            }

            if (iIndex==m_iIterPos+1)
            {
                m_iIterPos=iIndex;
                m_pIterNode=m_pIterNode->m_pNext;
                return KeyPair(m_pIterNode->m_KeyPair.m_Key, m_pIterNode->m_KeyPair.m_Value);
            }

            if (iIndex==m_iIterPos-1)
            {
                m_iIterPos=iIndex;
                m_pIterNode=m_pIterNode->m_pPrev;
                return KeyPair(m_pIterNode->m_KeyPair.m_Key, m_pIterNode->m_KeyPair.m_Value);
            }

            int iDistanceFromIterPos=m_iIterPos-iIndex;
            if (iDistanceFromIterPos < 0)
                iDistanceFromIterPos = - iDistanceFromIterPos;
            int iDistanceFromStart=iIndex;
            int iDistanceFromEnd=(m_iSize-1)-iIndex;

            if (m_iIterPos>=0 && iDistanceFromIterPos<iDistanceFromEnd && iDistanceFromIterPos<iDistanceFromEnd)
            {
                while (m_iIterPos<iIndex)
                {
                    m_pIterNode=m_pIterNode->m_pNext;
                    m_iIterPos++;
                }
                while (m_iIterPos>iIndex)
                {
                    m_pIterNode=m_pIterNode->m_pPrev;
                    m_iIterPos--;
                }
            }
            else
            {
                if (iDistanceFromStart<iDistanceFromEnd)
                {
                    m_pIterNode=m_pFirst;
                    for (int i=0; i<iIndex; i++)
                        m_pIterNode=m_pIterNode->m_pNext;
                }
                else
                {
                    m_pIterNode=m_pLast;
                    for (int i=0; i<iDistanceFromEnd; i++)
                    {
                        m_pIterNode=m_pIterNode->m_pPrev;
                    }
                }
            }

            m_iIterPos=iIndex;

        #ifdef _DEBUG_CHECKS
            CheckAll();
        #endif
            return KeyPair(m_pIterNode->m_KeyPair.m_Key, m_pIterNode->m_KeyPair.m_Value);
        }

        void Add(const TKey& Key, const TValue& Value)
        {
            AddInternal(Key, Value, false);
        }

        void Set(const TKey& Key, const TValue& Value)
        {
            AddInternal(Key, Value, true);
        }

        // Add an item to the map
        void AddInternal(const TKey& Key, const TValue& Value, bool replace)
        {
            CNode* pNode = m_pRoot;
            CNode* pParent = nullptr;

            int iCompare=0;
            while (pNode != &m_Leaf)
            {
                pParent = pNode;
                iCompare = TKeyCompare::Compare(Key, pNode->m_KeyPair.m_Key);

                if (iCompare < 0)
                    pNode = pNode->m_pLeft;
                else if (iCompare > 0)
                    pNode = pNode->m_pRight;
                else
                {
                    assert(replace && "Key already existings in map");

                    // Found a duplicate, replace it. We replace the key too, since
                    // equivalence is not always exact (e.g. case insensitive strings)
                    pNode->m_KeyPair.m_Value = Value;
                    pNode->m_KeyPair.m_Key = Key;
                    #ifdef _DEBUG_CHECKS
                    CheckAll();
                    #endif
                    return;
                }
            }

            CNode* pNew = m_NodePlex.Alloc();
            pNew->m_pParent = pParent;
            pNew->m_pLeft = &m_Leaf;
            pNew->m_pRight = &m_Leaf;
            pNew->m_bRed = true;
            pNew->m_KeyPair.m_Value = Value;
            pNew->m_KeyPair.m_Key = Key;

            if (pParent)
            {
                if (iCompare<0)
                {
                    pParent->m_pLeft = pNew;
                    pNew->m_pNext=pParent;
                    pNew->m_pPrev=pParent->m_pPrev;
                }
                else if (iCompare>0)
                {
                    pParent->m_pRight = pNew;
                    pNew->m_pPrev=pParent;
                    pNew->m_pNext=pParent->m_pNext;
                }
                else
                {
                    assert(false);
                }
            }
            else
            {
                m_pRoot = pNew;
                pNew->m_pPrev=nullptr;
                pNew->m_pNext=nullptr;
            }

            // Fix up traverse links
            if (pNew->m_pPrev)
                pNew->m_pPrev->m_pNext=pNew;
            else
                m_pFirst=pNew;

            if (pNew->m_pNext)
                pNew->m_pNext->m_pPrev=pNew;
            else
                m_pLast=pNew;

            if (m_pIterNode)
            {
                int iCompare=TKeyCompare::Compare(Key, m_pIterNode->m_KeyPair.m_Key);

                assert(iCompare!=0);

                // If the new key is before the current iterate position, update the iterate position
                if (iCompare<0)
                {
                    m_pIterNode=m_pIterNode->m_pPrev;
                }
            }

            // Now rebalance the tree.

            pNode = pNew;

            while (pNode != m_pRoot && pNode->m_pParent->m_bRed)
            {
                pParent = pNode->m_pParent;
                CNode* pGrandParent = pParent->m_pParent;

                if (pParent == pGrandParent->m_pLeft)
                {
                    CNode* pUncle = pGrandParent->m_pRight;

                    if (pUncle->m_bRed)
                    {
                        pParent->m_bRed = false;
                        pUncle->m_bRed = false;
                        pGrandParent->m_bRed = true;
                        pNode = pGrandParent;
                    }
                    else
                    {
                        if (pNode == pParent->m_pRight)
                        {
                            pNode = pParent;
                            RotateLeft(pNode);
                        }

                        pNode->m_pParent->m_bRed = false;
                        pNode->m_pParent->m_pParent->m_bRed = true;
                        RotateRight(pNode->m_pParent->m_pParent);
                    }
                }
                else
                {
                    CNode* pUncle = pGrandParent->m_pLeft;

                    if (pUncle->m_bRed)
                    {
                        pParent->m_bRed = false;
                        pUncle->m_bRed = false;
                        pGrandParent->m_bRed = true;
                        pNode = pGrandParent;
                    }
                    else
                    {
                        if (pNode == pParent->m_pLeft)
                        {
                            pNode = pParent;
                            RotateRight(pNode);
                        }

                        pNode->m_pParent->m_bRed = false;
                        pNode->m_pParent->m_pParent->m_bRed = true;
                        RotateLeft(pNode->m_pParent->m_pParent);
                    }
                }
            }

            m_pRoot->m_bRed = false;
            m_iSize++;

            #ifdef _DEBUG_CHECKS
            CheckAll();
            #endif
        }

        // Remove an item from the map
        void Remove(const TKey& Key)
        {
    #ifdef _DEBUG_CHECKS
            CheckAll();
    #endif

            CNode* z = m_pRoot;

            while (z != &m_Leaf)
            {
                int iCompare = TKeyCompare::Compare(Key, z->m_KeyPair.m_Key);

                if (iCompare < 0)
                    z = z->m_pLeft;
                else if (iCompare > 0)
                    z = z->m_pRight;
                else
                    break;
            }

            if (z == &m_Leaf)
                return;

            CNode* y = (z->m_pLeft == &m_Leaf || z->m_pRight == &m_Leaf) ? z : nextNode(z);

            CNode* x = (y->m_pLeft != &m_Leaf) ? y->m_pLeft : y->m_pRight;

            // Ensure that x->m_pParent is correct.
            // This is needed in case x == &m_Leaf

            x->m_pParent = y->m_pParent;

            if (y != m_pRoot)
            {
                if (y->m_pParent->m_pLeft == y)
                    y->m_pParent->m_pLeft = x;
                else
                    y->m_pParent->m_pRight = x;
            }
            else
                m_pRoot = x;


            if (m_pIterNode)
            {
                int iCompare = TKeyCompare::Compare(Key, m_pIterNode->m_KeyPair.m_Key);
                if (iCompare <= 0)
                {
                    m_pIterNode = m_pIterNode->m_pNext;
                }
            }

            if (y != z)
            {
                // deleting value in z, but keeping z node and moving value from y node
                z->m_KeyPair.m_Key = y->m_KeyPair.m_Key;
                z->m_KeyPair.m_Value = y->m_KeyPair.m_Value;
                z->m_pNext = y->m_pNext;
                if (z->m_pNext)
                    z->m_pNext->m_pPrev = z;
                else
                    m_pLast = z;

                if (m_pIterNode == y)
                    m_pIterNode = z;
            }
            else
            {
                // Update linked list
                if (z->m_pPrev)
                    z->m_pPrev->m_pNext = z->m_pNext;
                else
                    m_pFirst = z->m_pNext;

                if (z->m_pNext)
                    z->m_pNext->m_pPrev = z->m_pPrev;
                else
                    m_pLast = z->m_pPrev;

                if (m_pIterNode == z)
                {
                    m_pIterNode = m_pIterNode->m_pNext;
                    if (!m_pIterNode)
                    {
                        m_iIterPos = -1;
                    }
                }
            }

            // Rebalance the tree (see page 274 of Introduction to Algorithms)

            if (!y->m_bRed)
            {
                CNode* pNode = x;

                while (pNode != m_pRoot && !pNode->m_bRed)
                {
                    if (pNode == pNode->m_pParent->m_pLeft)
                    {
                        CNode* pSibling = pNode->m_pParent->m_pRight;

                        if (pSibling->m_bRed)
                        {
                            // Case 1: Sibling is m_bRed
                            pSibling->m_bRed = false;
                            pNode->m_pParent->m_bRed = true;
                            RotateLeft(pNode->m_pParent);
                            pSibling = pNode->m_pParent->m_pRight;
                        }
                        if (!pSibling->m_pLeft->m_bRed && !pSibling->m_pRight->m_bRed)
                        {
                            // Case 2: Sibling and its children are all black
                            pSibling->m_bRed = true;
                            pNode = pNode->m_pParent;
                            continue;
                        }
                        else if (!pSibling->m_pRight->m_bRed)
                        {
                            // Case 3: Sibling and its right child are both black
                            pSibling->m_pLeft->m_bRed = false;
                            pSibling->m_bRed = true;
                            RotateRight(pSibling);
                            pSibling = pNode->m_pParent->m_pRight;
                        }

                        // Case 4: Sibling and its left child are both black
                        pSibling->m_bRed = pNode->m_pParent->m_bRed;
                        pNode->m_pParent->m_bRed = false;
                        pSibling->m_pRight->m_bRed = false;
                        RotateLeft(pNode->m_pParent);
                        pNode = m_pRoot;
                    }
                    else
                    {
                        CNode* pSibling = pNode->m_pParent->m_pLeft;

                        if (pSibling->m_bRed)
                        {
                            // Case 5: Sibling is m_bRed
                            pSibling->m_bRed = false;
                            pNode->m_pParent->m_bRed = true;
                            RotateRight(pNode->m_pParent);
                            pSibling = pNode->m_pParent->m_pLeft;
                        }
                        if (!pSibling->m_pLeft->m_bRed && !pSibling->m_pRight->m_bRed)
                        {
                            // Case 6: Sibling and its children are all black
                            pSibling->m_bRed = true;
                            pNode = pNode->m_pParent;
                            continue;
                        }
                        else if (!pSibling->m_pLeft->m_bRed)
                        {
                            // Case 7: Sibling and its left child are both black
                            pSibling->m_pRight->m_bRed = false;
                            pSibling->m_bRed = true;
                            RotateLeft(pSibling);
                            pSibling = pNode->m_pParent->m_pLeft;
                        }

                        // Case 8: Sibling and its right child are both black
                        pSibling->m_bRed = pNode->m_pParent->m_bRed;
                        pNode->m_pParent->m_bRed = false;
                        pSibling->m_pLeft->m_bRed = false;
                        RotateRight(pNode->m_pParent);
                        pNode = m_pRoot;
                    }
                }

                pNode->m_bRed = false;
            }

            m_NodePlex.Free(y);
            m_iSize--;

    #ifdef _DEBUG_CHECKS
            CheckAll();
    #endif
        }

        // Remove all items from the map
        void Clear()
        {
            FreeNode(m_pRoot);
            m_pRoot = &m_Leaf;
            m_pFirst = nullptr;
            m_pLast = nullptr;
            m_iSize = 0;
            m_iIterPos = -1;
            m_pIterNode = nullptr;

    #ifdef _DEBUG_CHECKS
            CheckAll();
    #endif
        }

        // Get an item from the map, return default if doesn't exist
        const TValue& Get(const TKey& Key) const
        {
            CNode* pNode = FindNode(Key);
            assert(pNode != nullptr);
            return pNode->m_KeyPair.m_Value;
        }

        // Get an item from the map, return default if doesn't exist
        const TValue& Get(const TKey& Key, const TValue& Default) const
        {
            CNode* pNode = FindNode(Key);
            if (!pNode)
                return Default;
            return pNode->m_KeyPair.m_Value;
        }

		// Shortcut to above
		const TValue& operator[](TKey key) const
		{
			return GetValue(key);
		}

        // Find an item in the map and return true/false if found or not
        bool TryGetValue(const TKey& Key, TValue& Value) const
        {
            CNode* pNode = FindNode(Key);
            if (!pNode)
                return false;
            Value = pNode->m_KeyPair.m_Value;
            return true;
        }

        // Check if the map contains a key
        bool ContainsKey(const TKey& Key) const
        {
            return FindNode(Key) != nullptr;
        }

    #ifdef _DEBUG
        void CheckAll()
        {
            CheckTree();
            CheckChain();
        }
    #endif

        // Implementation
    protected:

        // CKeyPairInternal
        struct CKeyPairInternal
        {
            TKey	m_Key;
            TValue	m_Value;
        };

        // CNode
        struct CNode
        {
            CKeyPairInternal	m_KeyPair;
            CNode* m_pParent;
            CNode* m_pLeft;
            CNode* m_pRight;
            CNode* m_pPrev;
            CNode* m_pNext;
            bool	m_bRed;
        };

        // Operations
    #ifdef _DEBUG
        void CheckChain()
        {
            if (m_pFirst)
            {
                assert(m_pFirst->m_pPrev == nullptr);
                assert(m_pLast != nullptr);
                assert(m_pLast->m_pNext == nullptr);

                int i = 0;
                CNode* pNode = m_pFirst;
                while (pNode)
                {
                    if (pNode->m_pPrev)
                    {
                        assert(pNode->m_pPrev->m_pNext == pNode);
                    }
                    else
                    {
                        assert(pNode == m_pFirst);
                    }

                    if (pNode->m_pNext)
                    {
                        // Check order
                        int iCompare = TKeyCompare::Compare(pNode->m_KeyPair.m_Key, pNode->m_pNext->m_KeyPair.m_Key);
                        assert(iCompare < 0);

                        assert(pNode->m_pNext->m_pPrev == pNode);
                    }
                    else
                    {
                        assert(pNode == m_pLast);
                    }

                    if (i == m_iIterPos)
                    {
                        assert(m_pIterNode == pNode);
                    }

                    pNode = pNode->m_pNext;
                    i++;
                }

                if (m_iIterPos >= 0)
                {
                    assert(m_pIterNode != nullptr);
                }


                assert(i == m_iSize);
            }

        }

        bool CheckTree(CNode* pNode = nullptr)
        {
            int lh = 1, rh = 1;

            if (!pNode)
                pNode = m_pRoot;

            if (pNode->m_pLeft != &m_Leaf)
                lh = CheckTree(pNode->m_pLeft);

            if (pNode->m_pRight != &m_Leaf)
                rh = CheckTree(pNode->m_pRight);

            assert(lh == rh);

            return !!(lh + !pNode->m_bRed);
        }
    #endif

        // Free a node
        void FreeNode(CNode* pNode)
        {
            if (pNode && pNode != &m_Leaf)
            {
                FreeNode(pNode->m_pLeft);
                FreeNode(pNode->m_pRight);
                m_NodePlex.Free(pNode);
            }
        }

        // Find the next node
        CNode* nextNode(CNode* pNode)
        {
            if (pNode->m_pRight != &m_Leaf)
            {
                pNode = pNode->m_pRight;

                while (pNode->m_pLeft != &m_Leaf)
                    pNode = pNode->m_pLeft;

                return pNode;
            }

            CNode* pParent = pNode->m_pParent;

            while (pParent != &m_Leaf && pNode == &m_Leaf)
            {
                pNode = pParent;
                pParent = pParent->m_pParent;
            }

            return pParent;
        }

        // Rotate tree left
        void RotateLeft(CNode* x)
        {
            CNode* parent = m_Leaf.m_pParent;
            CNode* y = x->m_pRight;

            // Turn y's left subtree into x's right subtree

            x->m_pRight = y->m_pLeft;
            x->m_pRight->m_pParent = x;

            // Link x's parent to y

            y->m_pParent = x->m_pParent;

            if (x != m_pRoot)
            {
                if (x->m_pParent->m_pLeft == x)
                    x->m_pParent->m_pLeft = y;
                else
                    x->m_pParent->m_pRight = y;
            }
            else
                m_pRoot = y;

            // Put x on y's left

            y->m_pLeft = x;
            x->m_pParent = y;
            m_Leaf.m_pParent = parent;
        }

        // Rotate tree right
        void RotateRight(CNode* y)
        {
            CNode* parent = m_Leaf.m_pParent;
            CNode* x = y->m_pLeft;

            // Turn x's right subtree into y's left subtree

            y->m_pLeft = x->m_pRight;
            y->m_pLeft->m_pParent = y;

            // Link y's parent to x

            x->m_pParent = y->m_pParent;

            if (y != m_pRoot)
            {
                if (y->m_pParent->m_pLeft == y)
                    y->m_pParent->m_pLeft = x;
                else
                    y->m_pParent->m_pRight = x;
            }
            else
                m_pRoot = x;

            // Put y on x's right

            x->m_pRight = y;
            y->m_pParent = x;
            m_Leaf.m_pParent = parent;
        }

        // Find a node with specified key
        CNode* FindNode(const TKey& Key) const
        {
            CNode* pNode = m_pRoot;

            while (pNode != &m_Leaf)
            {
                int iCompare = TKeyCompare::Compare(Key, pNode->m_KeyPair.m_Key);

                if (iCompare < 0)
                    pNode = pNode->m_pLeft;
                else if (iCompare > 0)
                    pNode = pNode->m_pRight;
                else
                    return pNode;
            }

            return nullptr;
        }



        // Attributes
        Plex<CNode>	m_NodePlex;
        CNode* m_pRoot;
        CNode* m_pFirst;
        CNode* m_pLast;
        CNode			m_Leaf;
        mutable int		m_iIterPos;
        mutable CNode* m_pIterNode;
        int				m_iSize;

    private:
        // Unsupported
        Dictionary(const Dictionary& Other);
        Dictionary& operator=(const Dictionary& Other);
    };

}
