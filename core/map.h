#pragma once

#include "Semantics.h"
#include "Compare.h"
#include "HashCore.h"
#include "PlacedConstructor.h"

namespace SimpleLib
{

template <typename TKey, typename TValue, typename TKeyCompare = SDefaultCompare>
class Map
{
    typedef typename get_semantics<TKey>::TSemantics TKeySemantics;
    typedef typename TKeySemantics::TArg TKeyArg;
    typedef typename get_semantics<TValue>::TSemantics TValueSemantics;
    typedef typename TValueSemantics::TArg TValueArg;
public: 
    // Constructor
    Map()
    {
    }

    // Destructor
    virtual ~Map()
    {
        Clear();
    }

    // No copy
    Map(const Map&) = delete;
    Map& operator=(const Map&) = delete;		

    Map(Map&& other)
        : core(SimpleLib::move(other.core))
    {
    }

    Map& operator=(Map&& other)
    {
        core = SimpleLib::move(other.core);
        return *this;
    }


    // Get number of elements in map
    int GetCount() const 
    {
        return core.GetCount();
    }

    // Is the map empty?
    bool IsEmpty() const
    {
        return core.GetCount() == 0;
    }

    // Add a new key to map, asserts if already exists
    void Add(const TKeyArg& Key, const TValueArg& Value)
    {
        AddInternal(Key, Value, false);
    }

    // Set a key to a value, replaces if already exists
    void Set(const TKeyArg& Key, const TValueArg& Value)
    {
        AddInternal(Key, Value, true);
    }

    // Remove an item from the map
    bool Remove(const TKeyArg& Key)
    {
        void* pOldKey;
        void* pOldValue;
        if (core.Remove(&Key, pOldKey, pOldValue))
        {
            Destructor((TKey*)pOldKey);
            Destructor((TValue*)pOldValue);
            return true;
        }
        return false;
    }

    // Remove all items from the map
    void Clear()
    {
        int count = core.get_table_count();
        for (int i=0; i<count; i++)
        {
            void* pKey;
            void* pValue;
            if (core.get_table_entry(i, pKey, pValue))
            {
                Destructor((TKey*)pKey);
                Destructor((TValue*)pValue);
            }
        }
        core.Clear();
    }

    class Iter
    {
    public:
        const TKey& GetKey() { return *_key; };
        const TValue& GetValue() { return *_value; }

        bool Next() { return _owner->GetNext(*this); }

    private:
        Iter(Map* owner, int version)
        {
            _owner = owner;
            _version = version;
        }

        Iter(const Iter& other)
        {
            _owner = other._owner;
            _pos = other._pos;
            _version = other._version;
            _key = other._key;
            _value = other._value;
        }

        const TKey* _key = nullptr;
        const TValue* _value = nullptr;
        Map* _owner;
        int _pos = -1;
        int _version = 0;
        friend class Map;
    };

    Iter Iterate()
    {
        return Iter(this, core.get_table_version());
    }

    bool GetNext(Iter& iter)
    {
        // Check not modified
        assert(core.get_table_version() == iter._version);

        int count = core.get_table_count();

        iter._pos++;
        while (iter._pos < count)
        {
            void* key;
            void* value;
            if (core.get_table_entry(iter._pos, key, value))
            {
                iter._key = (TKey*)key;
                iter._value = (TValue*)value;
                return true;
            }
            iter._pos++;
        }
        return false;
    }

    // Get an item from the map, assert if not found
    const TValue& Get(const TKeyArg& Key) const
    {
        const TValue* val = (const TValue*)core.Find(&Key);
        assert(val != nullptr);
        return *val;
    }

    // Shortcut to above
    const TValue& operator[](const TKeyArg& key) const
    {
        return GetValue(key);
    }

    // Get an item from the map, return default if doesn't exist
    const TValue& Get(const TKeyArg& Key, const TValueArg& Default) const
    {
        const TValue* val = (const TValue*)core.Find(&Key);
        if (val)
        {
            return *val;
        }
        return Default;
    }

    // Find an item in the map and return true/false if found or not
    bool TryGetValue(const TKeyArg& Key, TValue& Value) const
    {
        const TValue* val = (const TValue*)core.Find(&Key);
        if (val)
        {
            Constructor(&Value, *val);
            return true;
        }
        return false;
    }

    // Check if a map contains a key
    bool ContainsKey(const TKeyArg& Key) const
    {
        return core.Find(&Key) != nullptr;
    }

    // Implementation
private:
    class Core : public HashCore
    {
    public:
        Core() : 
            HashCore(sizeof(TKey), sizeof(TValue))
        {
        };
        virtual ~Core() 
        {
        };
        virtual uint32_t HashKey(const void* a) const override
        {
            return TKeyCompare::Hash(*(const TKey*)a);
        }
        virtual bool KeyEq(const void* a, const void* b) const override
        {
            return TKeyCompare::AreEqual(*(const TKey*)a, *(const TKey*)b);
        }
    };
    Core core;

private:
    // Internal helper to add item to map
    void AddInternal(const TKeyArg& Key, const TValueArg& Value, bool replace)
    {
        void* pValue;
        void* pKey;
        if (core.Add(&Key, replace, pKey, pValue))
        {
            Constructor((TKey*)pKey, Key);
            Constructor((TValue*)pValue, Value);
        }
    }
};




}