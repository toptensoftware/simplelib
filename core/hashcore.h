#pragma once

#include <stdint.h>
#include <malloc.h>
#include <string.h>
#include <assert.h>

#include "HashUtils.h"

namespace SimpleLib
{


class HashCore
{
public:
    // Constructor
    HashCore(size_t keySize, size_t valueSize)
    {
        m_keySize = keySize;
        m_valueSize = valueSize;
        m_entrySize = keySize + valueSize + sizeof(entry);
        m_version = 0;
        m_freecount = 0;
        m_freelist = -1;
        m_count = 0;
        m_capacity = 0;
        m_hashtable = nullptr;
        m_entries = nullptr;
    };

	// No copy
	HashCore(const HashCore&) = delete;
	HashCore& operator=(const HashCore&) = delete;		


	// Move
	HashCore(HashCore&& other)
	{
        m_keySize = other.m_keySize;
        m_valueSize = other.m_valueSize;
        m_entrySize = other.m_entrySize;
        m_version = other.m_version;
        m_freecount = other.m_freecount;
        m_freelist = other.m_freelist;
        m_count = other.m_count;
        m_capacity = other.m_capacity;
        m_hashtable = other.m_hashtable;
        m_entries = other.m_entries;

        other.m_entries = nullptr;
        other.Reset();
	}

	// Move
	HashCore& operator=(HashCore&& other)
	{
		if (this == &other)
			return *this;

        Reset();

        m_keySize = other.m_keySize;
        m_valueSize = other.m_valueSize;
        m_entrySize = other.m_entrySize;
        m_version = other.m_version;
        m_freecount = other.m_freecount;
        m_freelist = other.m_freelist;
        m_count = other.m_count;
        m_capacity = other.m_capacity;
        m_hashtable = other.m_hashtable;
        m_entries = other.m_entries;

        other.m_entries = nullptr;
        other.Reset();

		return *this;    
	}


    // Destructor
    virtual ~HashCore()
    {
        Reset();
    }

    // Free and reset everything back to initial state
    void Reset()
    {
        free(m_entries);
        m_version = 0;
        m_freecount = 0;
        m_freelist = -1;
        m_count = 0;
        m_capacity = 0;
        m_hashtable = nullptr;
        m_entries = nullptr;
    }

    // Resize to a specific capacity
    // (actual capacity will be next largest prime)
    void Resize(int capacity)
    {
        // Work out prime capacity
        capacity = (int)next_prime_capacity(capacity);

        // Resize entries array
        m_entries = (char*)realloc(m_entries, capacity * (m_entrySize + sizeof(int)));
        m_hashtable = (int*)(m_entries + capacity * (m_entrySize));
        m_capacity = capacity;

        // Resize hashtable
        ClearHashTable(m_hashtable, m_capacity);

        // Rebuild hash table
        int used = m_count + m_freecount;
        for (int i = 0; i < used; i++)
        {
            entry* e = get_entry(i);
            if (e->hashcode >= 0)
            {
                int bucket = e->hashcode % m_capacity;
                e->next = m_hashtable[bucket];
                m_hashtable[bucket] = i;
            }
        }
    }

    // Add a key to the hash map, returning pointer to storage
    // of element value.   If replace is false and key already
    // exists, returns nullptr
    bool Add(const void* key, bool replace, void*& pKey, void*& pValue)
    {
        // Make initialized
        if (m_capacity == 0)
            Resize(1);

        // Hash
        int hashcode = HashKey(key) & 0x7FFFFFFF;
        int bucket = hashcode % m_capacity;

        // Look for existing entry
        int index = m_hashtable[bucket];
        while (index >= 0)
        {
            // Get the entry at this slot
            entry* e = get_entry(index);

            // Same key?
            if (e->hashcode == hashcode && KeyEq(e->data, key))
            {
                if (!replace)
                    return false;
                m_version++;
                return e->data + m_keySize;
            }

            // Move to next
            index = e->next;
        }

        // Not found, create a new entry
        index = AllocEntry();

        // Recalculate bucket in case capacity changed
        bucket = hashcode % m_capacity;

        // Get the entry
        entry* e = get_entry(index);

        // Setup entry
        e->hashcode = hashcode;
        e->next = m_hashtable[bucket];
        m_hashtable[bucket] = index;

        // Update counts
        m_version++;

        pKey = e->data;
        pValue = e->data + m_keySize;
        return true;
    }

    // Remove all entries, keeping allocated capacity
    void Clear()
    {
        m_count = 0;
        m_freecount = 0;
        m_freelist = -1;
        m_version++;
        ClearHashTable(m_hashtable, m_capacity);
    }

    // Remove a specified key.
    // If not found returns null
    // If found, returns pointer to the key value
    // which is only valid until next operation on this instance
    bool Remove(const void* key, void*& pOldKey, void*& pOldValue)
    {
        if (m_capacity == 0)
            return false;

        // Hash
        int hashcode = HashKey(key) & 0x7FFFFFFF;
        int bucket = hashcode % m_capacity;

        // Look for existing entry
        int index = m_hashtable[bucket];
        entry* eprev = nullptr;
        while (index >= 0)
        {
            // Get the entry at this slot
            entry* e = get_entry(index);

            // Same key?
            if (e->hashcode == hashcode && KeyEq(e->data, key))
            {
                // Remove from chain
                if (eprev == nullptr)
                {
                    m_hashtable[bucket] = e->next;
                }
                else
                {
                    eprev->next = e->next;
                }

                // Add to free list
                e->next = m_freelist;
                e->hashcode = -1;
                m_freelist = index;
                m_freecount++;
                m_count--;
                m_version++;

                pOldValue = e->data + m_keySize;
                pOldKey = e->data;
                return true;
            }

            // Move to next
            eprev = e;
            index = e->next;
        }

        // Not found
        return false;
    }

    // Look up a key, returning null if not found or
    // pointer to value storage
    void* Find(const void* key) const
    {
        if (m_capacity == 0)
            return 0;

        // Hash
        int hashcode = HashKey(key) & 0x7FFFFFFF;
        int bucket = hashcode % m_capacity;

        // Look for existing entry
        int index = m_hashtable[bucket];
        int prev = -1;
        while (index >= 0)
        {
            // Get the entry at this slot
            entry* e = get_entry(index);

            // Same key?
            if (e->hashcode == hashcode && KeyEq(e->data, key))
            {
                return e->data + m_keySize;	
            }

            // Move to next
            index = e->next;
        }

        // Not found
        return nullptr;
    }

    // Get number of items in map
    int GetCount() const
    {
        return m_count;
    }

    // Get capacity of map
    int GetCapacity() const
    {
        return m_capacity;
    }

    // Ensure there is capacity
    int Reserve(int capacity)
    {
        if (capacity > m_capacity)
            Resize(capacity);

        return m_capacity;
    }

    // Enumerate the map
    bool Enumerate(void* user, bool (*callback)(void* user, const void* key, void* value))
    {
        int index = 0;
        int version = m_version;
        int used = m_count + m_freecount;
        while (index < used)
        {
            entry* e = get_entry(index);
            if (e->hashcode >= 0)
            {
                // Invoke callback
                if (!callback(user, e->data, e->data + m_keySize))
                    return false;

                // Check if modified
                if (version != m_version)
                    return false;
            }
            index++;
        }

        return true;
    }

    int get_table_count() const
    {
        return m_count + m_freecount;
    }

    bool get_table_entry(int index, void*& pKey, void*& pValue) const
    {
        assert(index >= 0 && index < get_table_count());
        entry* e = get_entry(index);
        if (e->hashcode >= 0)
        {
            pValue = e->data + m_keySize;
            pKey = e->data;        
            return true;
        }
        return false;
    }

    int get_table_version()
    {
        return m_version;
    }

protected:

    // Client provided hash functions
	virtual uint32_t HashKey(const void* a) const = 0;
	virtual bool KeyEq(const void* a, const void* b) const = 0;


    // Hash map entry
    #pragma warning(disable: 4200)
    struct entry
    {
        int next;
        int hashcode;
        char data[];		// key first, value second
    };
    #pragma warning(default: 4200)

    // Get pointer to Nth entry
    entry* get_entry(int index) const
    {
        return (entry*)(m_entries + (index) * m_entrySize);
    }


	size_t m_keySize;       // Size of key data
	size_t m_valueSize;     // Size of value data
	size_t m_entrySize;     // Size of key + value + entry
	int m_version;          // For iteration change checks
	int m_freelist;         // First entry in the free list
	int m_freecount;        // Number of entries in the free list
	int m_count;            // Number of entries in the map
	int m_capacity;         // Total capacity of the map
	char* m_entries;        // Pointer to the entry table
	int* m_hashtable;       // Pointer to the hash table (after entry table)

    // Set all entries in hash table to -1
    void ClearHashTable(int* hashmap, int capacity)
    {
        memset(m_hashtable, 0xFF, capacity * sizeof(int));
    }

    // Allocate an entry either from the free list or next available
    int AllocEntry()
    {
        // Remove entry from free list
        if (m_freelist >= 0)
        {
            int index = m_freelist;
            entry* e = get_entry(index);
            m_freelist = e->next;
            m_freecount--;
            m_count++;
            return index;
        }

        // Room at highwater mark?
        assert(m_freecount == 0);
        if (m_count >= m_capacity)
        {
            Resize(m_capacity * 2);
        }

        // Allocate item at high water
        return m_count++;
    }
};


}