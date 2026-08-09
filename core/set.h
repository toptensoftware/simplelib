#pragma once

#include "Semantics.h"
#include "Compare.h"
#include "HashCore.h"
#include "PlacedConstructor.h"

namespace SimpleLib
{

template <typename T, typename TCompare = SDefaultCompare>
class Set
{
    typedef typename get_semantics<T>::TSemantics TSemantics;
    typedef typename TSemantics::TArg TArg;
public: 
    // Constructor
    Set()
    {
    }

    Set(Set&& other)
        : core(SimpleLib::move(other.core))
    {
    }

    Set& operator=(Set&& other)
    {
        core = SimpleLib::move(other.core);
        return *this;
    }

    // No copy
    Set(const Set&) = delete;
    Set& operator=(const Set&) = delete;		



    // Destructor
    virtual ~Set()
    {
        Clear();
    }

    // Get number of elements in Set
    int GetCount() const 
    {
        return core.GetCount();
    }

    // Is the Set empty?
    bool IsEmpty() const
    {
        return core.GetCount() == 0;
    }

    // Set a key to a value, replaces if already exists
    void Add(const TArg& Key)
    {
        void* pValue;
        void* pKey;
        bool bExisted;
        if (core.Add(&Key, true, pKey, pValue, &bExisted))
        {
            if (bExisted)
                Destructor((T*)pKey);
            Constructor((T*)pKey, Key);
        }
    }

    // Remove an item from the Set
    bool Remove(const TArg& Key)
    {
        void* pOldKey;
        void* pOldValue;
        if (core.Remove(&Key, pOldKey, pOldValue))
        {
            Destructor((T*)pOldKey);
            return true;
        }
        return false;
    }

    // Remove all items from the Set
    void Clear()
    {
        int count = core.get_table_count();
        for (int i=0; i<count; i++)
        {
            void* pKey;
            void* pValue;
            if (core.get_table_entry(i, pKey, pValue))
            {
                Destructor((T*)pKey);
            }
        }
        core.Clear();
    }

    class Iter
    {
    public:
        const T& Get() { return *_key; };

        bool Next() { return _owner->GetNext(*this); }

    private:
        Iter(Set* owner, int version)
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
        }

        const T* _key = nullptr;
        Set* _owner;
        int _pos = -1;
        int _version = 0;
        friend class Set;
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
                iter._key = (T*)key;
                return true;
            }
            iter._pos++;
        }
        return false;
    }

    // Check if a Set contains a key
    bool Contains(const TArg& Key) const
    {
        return core.Find(&Key) != nullptr;
    }

    // Implementation
private:
    class Core : public HashCore
    {
    public:
        Core() :
            HashCore(sizeof(T), 0)
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
            return TCompare::Hash(*(const T*)a);
        }
        virtual bool KeyEq(const void* a, const void* b) const override
        {
            return TCompare::AreEqual(*(const T*)a, *(const T*)b);
        }
    };
    Core core;

};




}