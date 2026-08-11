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
    typedef typename TKeySemantics::TStorage TKeyStorage;
    typedef typename get_semantics<TValue>::TSemantics TValueSemantics;
    typedef typename TValueSemantics::TArg TValueArg;
    typedef typename TValueSemantics::TStorage TValueStorage;
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
    void Add(TKeyArg Key, TValueArg Value)
    {
        AddInternal(Key, Value, false);
    }

    // Set a key to a value, replaces if already exists
    void Set(TKeyArg Key, TValueArg Value)
    {
        AddInternal(Key, Value, true);
    }

    // Remove an item from the map
    bool Remove(TKeyArg Key)
    {
        void* pOldKey;
        void* pOldValue;
        if (core.Remove(&Key, pOldKey, pOldValue))
        {
            Destructor((TKeyStorage*)pOldKey);
            Destructor((TValueStorage*)pOldValue);
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
                Destructor((TKeyStorage*)pKey);
                Destructor((TValueStorage*)pValue);
            }
        }
        core.Clear();
    }

    class Iter
    {
    public:
        TKeyArg GetKey() { return *_key; };
        TValueArg GetValue() { return *_value; }

        bool Next() { return _owner->GetNext(*this); }

    private:
        Iter(Map* owner, bool forward, int version)
        {
            _owner = owner;
            _forward = forward;
            _version = version;
        }

        Iter(const Iter& other)
        {
            _owner = other._owner;
            _forward = other._forward;
            _pos = other._pos;
            _version = other._version;
            _key = other._key;
            _value = other._value;
        }

        const TKeyStorage* _key = nullptr;
        const TValueStorage* _value = nullptr;
        Map* _owner;
        int _pos = -1;
        int _version = 0;
        bool _forward = false;
        friend class Map;
    };

    Iter Iterate()
    {
        return Iter(this, true, core.get_table_version());
    }

    Iter IterateReverse()
    {
        Iter iter(this, false, core.get_table_version());
        iter._pos = core.get_table_count();
        return iter;
    }

    bool GetNext(Iter& iter)
    {
        // Check not modified
        assert(core.get_table_version() == iter._version);

        if (iter._forward)
        {
            int count = core.get_table_count();

            iter._pos++;
            while (iter._pos < count)
            {
                void* key;
                void* value;
                if (core.get_table_entry(iter._pos, key, value))
                {
                    iter._key = (TKeyStorage*)key;
                    iter._value = (TValueStorage*)value;
                    return true;
                }
                iter._pos++;
            }
        }
        else
        {
            iter._pos--;
            while (iter._pos >= 0)
            {
                void* key;
                void* value;
                if (core.get_table_entry(iter._pos, key, value))
                {
                    iter._key = (TKeyStorage*)key;
                    iter._value = (TValueStorage*)value;
                    return true;
                }
                iter._pos--;
            }
        }
        return false;
    }


    List<TKeyArg> GetKeys()
    {
        List<TKeyArg> r;
        for (auto iter = Iterate(); iter.Next(); )
        {
            r.Add(iter.GetKey());
        }
        return r;
    }

    List<TValueArg> GetValues()
    {
        List<TValueArg> r;
        for (auto iter = Iterate(); iter.Next(); )
        {
            r.Add(iter.GetValue());
        }
        return r;
    }


    // Get an item from the map, assert if not found
    TValueArg Get(TKeyArg Key) const
    {
        const TValueStorage* val = (const TValueStorage*)core.Find(&Key);
        assert(val != nullptr);
        return *val;
    }

    // Shortcut to above
    TValueArg operator[](TKeyArg key) const
    {
        return Get(key);
    }

    // Get an item from the map, return default if doesn't exist
    TValueArg Get(TKeyArg Key, TValueArg Default) const
    {
        const TValueStorage* val = (const TValueStorage*)core.Find(&Key);
        if (val)
        {
            return *val;
        }
        return Default;
    }

    // Find an item in the map and return true/false if found or not
    bool TryGetValue(TKeyArg Key, TValueArg& Value) const
    {
        const TValueStorage* val = (const TValueStorage*)core.Find(&Key);
        if (val)
        {
            Value = *val;
            return true;
        }
        return false;
    }

    // Check if a map contains a key
    bool ContainsKey(TKeyArg Key) const
    {
        return core.Find(&Key) != nullptr;
    }

    // Implementation
private:
    class Core : public HashCore
    {
    public:
        Core() :
            HashCore(sizeof(TKeyStorage), sizeof(TValueStorage))
        {
        };
        virtual ~Core()
        {
        };

        // The user-declared destructor above suppresses the implicitly
        // generated move constructor/assignment, so provide them explicitly
        // (forwarding to HashCore's) rather than falling back to the
        // deleted copy constructor/assignment.
        Core(Core&& other) : HashCore(SimpleLib::move(other))
        {
        }
        Core& operator=(Core&& other)
        {
            HashCore::operator=(SimpleLib::move(other));
            return *this;
        }
        virtual uint32_t HashKey(const void* a) const override
        {
            return TKeyCompare::Hash(*(const TKeyStorage*)a);
        }
        virtual bool KeyEq(const void* a, const void* b) const override
        {
            return TKeyCompare::AreEqual(*(const TKeyStorage*)a, *(const TKeyStorage*)b);
        }
    };
    Core core;

private:
    // Internal helper to add item to map
    void AddInternal(TKeyArg Key, TValueArg Value, bool replace)
    {
        void* pValue;
        void* pKey;
        bool bExisted;
        if (core.Add(&Key, replace, pKey, pValue, &bExisted))
        {
            if (bExisted)
            {
                Destructor((TKeyStorage*)pKey);
                Destructor((TValueStorage*)pValue);
            }
            Constructor((TKeyStorage*)pKey, Key);
            Constructor((TValueStorage*)pValue, Value);
        }
    }
};




}