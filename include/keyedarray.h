#ifndef __simplelib_keyedarray_h__
#define __simplelib_keyedarray_h__

#include <assert.h>
#include <stdlib.h>

#include "semantics.h"
#include "vector.h"

namespace SimpleLib
{
    template <typename TKey, typename TValue, 
            typename TKeySem=SDefaultSemantics<TKey>::TSemantics, 
            typename TValueSem=SDefaultSemantics<TValue>::TSemantics>
	class CKeyedArray
	{
	public:

		typedef typename SArgType<TKey>::TArgType TKeyArg;
		typedef typename TKeySem::TCompare TKeyCompare;
        typedef CKeyedArray<TKey, TValue, TKeySem, TValueSem> _CKeyedArray;

		// Constructor
		CKeyedArray()
		{
		}

		// Destructor
		virtual ~CKeyedArray()
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
		void Remove(const TKeyArg& Key)
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
		TValue Detach(const TKeyArg& Key)
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
		class CKeyPair;
		CKeyPair GetAt(int index) const
		{
			return CKeyPair(m_Keys[index], m_Values[index]);
		}

		// Get by key, assert if unknown key
		const TValue& Get(const TKeyArg& Key) const
		{
			int index = IndexOf(Key);
			assert(index >= 0);
			return m_Values[index];
		}

		// Get by key, return default if unknown key
		const TValue& Get(const TKeyArg& Key, const TValue& Default) const
		{
			int index = IndexOf(Key);
			if (index < 0)
				return Default;

			return m_Values[index];
		}

		// Shortcut to above
		const TValue& operator[](TKeyArg key) const
		{
			return GetValue(key);
		}

		// Get to get a value
		bool TryGetValue(const TKeyArg& Key, TValue& Value) const
		{
			int index = IndexOf(Key);
			if (index < 0)
				return false;

			Value = m_Values[index];
			return true;
		}

		// Check if contains a key
		bool ContainsKey(const TKeyArg& Key) const
		{
			return IndexOf(Key) >= 0;
		}

		// Find index of key
		int IndexOf(const TKeyArg& Key) const
		{
			return m_Keys.IndexOf(Key);
		}

		// Type used as return value from GetAt
		class CKeyPair
		{
		public:
			CKeyPair(TKey& key, TValue& value) :
				Key(key),
				Value(value)
			{
			}
			CKeyPair(const CKeyPair& Other) :
				Key(Other.Key),
				Value(Other.Value)
			{
			}

			const TKey& Key;
			TValue& Value;

		private:
			CKeyPair& operator=(const CKeyPair& Other);
		};


	protected:
		CVector<TKey, TKeySem>			m_Keys;
		CVector<TValue, TValueSem>		m_Values;

	private:
		// Unsupported
		CKeyedArray(const CKeyedArray& Other);
		CKeyedArray& operator=(const CKeyedArray& Other);
	};


}	// namespace

#endif  // __simplelib_keyedarray_h__