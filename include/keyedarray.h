#ifndef __simplelib_keyedarray_h__
#define __simplelib_keyedarray_h__

#include <assert.h>
#include <stdlib.h>

#include "compare.h"
#include "list.h"

namespace SimpleLib
{
    template <typename TKey, typename TValue, typename TKeyCompare = SDefaultCompare>
	class KeyedArray
	{
	public:

		// Constructor
		KeyedArray()
		{
		}

		// Destructor
		virtual ~KeyedArray()
		{
		}

		// Get number of items
		int GetCount() const
		{
			return m_Keys.GetCount();
		}

		// Check if empty
		bool IsEmpty() const
		{
			return m_Keys.IsEmpty();
		}

		// Add an entry, assert if key already exists
		void Add(const TKey& Key, const TValue& Value)
		{
			int iPos = IndexOf(Key);
			assert(iPos < 0);
			m_Keys.Add(Key);
			m_Values.Add(Value);
		}

		// Add or replace entry
		void Set(const TKey& Key, const TValue& Value)
		{
			int index = IndexOf(Key);
			if (index < 0)
			{
				m_Keys.Add(Key);
				m_Values.Add(Value);
			}
			else
			{
				m_Keys.ReplaceAt(index, Key);
				m_Values.ReplaceAt(index, Value);
			}
		}

		// Set the value at a position
		void SetValueAt(int index, const TValue& Value)
		{
			m_Values.ReplaceAt(index, Value);
		}

		// Remove entry by key
		void Remove(const TKey& Key)
		{
			int index = IndexOf(Key);
			if (index >= 0)
			{
				RemoveAt(index);
			}
		}

		// Remove entry at index
		void RemoveAt(int index)
		{
			m_Keys.RemoveAt(index);
			m_Values.RemoveAt(index);
		}

		// Remove all
		void Clear()
		{
			m_Keys.Clear();
			m_Values.Clear();
		}

		// Detach
		TValue Detach(const TKey& Key)
		{
			int index = IndexOf(Key);
			return DetachAt(index);
		}

		// Detach at inex
		TValue DetachAt(int index)
		{
			assert(index >= 0);
			m_Keys.RemoveAt(index);
			return m_Values.DetachAt(index);
		}

		// Get a key at index
		const TKey& GetKeyAt(int index) const
		{
			return m_Keys.GetAt(index);
		}

		// Get a value at index
		const TValue& GetValueAt(int index) const
		{
			return m_Values.GetAt(index);
		}

		// Get key and value at index
		class KeyPair;
		KeyPair GetAt(int index) const
		{
			return KeyPair(m_Keys[index], m_Values[index]);
		}

		// Get by key, assert if unknown key
		const TValue& Get(const TKey& Key) const
		{
			int index = IndexOf(Key);
			assert(index >= 0);
			return m_Values[index];
		}

		// Get by key, return default if unknown key
		const TValue& Get(const TKey& Key, const TValue& Default) const
		{
			int index = IndexOf(Key);
			if (index < 0)
				return Default;

			return m_Values[index];
		}

		// Shortcut to above
		const TValue& operator[](TKey key) const
		{
			return GetValue(key);
		}

		// Get to get a value
		bool TryGetValue(const TKey& Key, TValue& Value) const
		{
			int index = IndexOf(Key);
			if (index < 0)
				return false;

			Value = m_Values[index];
			return true;
		}

		// Check if contains a key
		bool ContainsKey(const TKey& Key) const
		{
			return IndexOf(Key) >= 0;
		}

		// Find index of key
		int IndexOf(const TKey& Key) const
		{
			return m_Keys.IndexOf<TKeyCompare>(Key);
		}

		// Type used as return value from GetAt
		class KeyPair
		{
		public:
			KeyPair(TKey& key, TValue& value) :
				Key(key),
				Value(value)
			{
			}
			KeyPair(const KeyPair& Other) :
				Key(Other.Key),
				Value(Other.Value)
			{
			}

			const TKey& Key;
			TValue& Value;

		private:
			KeyPair& operator=(const KeyPair& Other);
		};


	protected:
		List<TKey>		m_Keys;
		List<TValue>		m_Values;

	private:
		// Unsupported
		KeyedArray(const KeyedArray& Other);
		KeyedArray& operator=(const KeyedArray& Other);
	};


}	// namespace

#endif  // __simplelib_keyedarray_h__