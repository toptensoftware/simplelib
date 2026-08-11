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
    typedef typename TSemantics::TStorage TStorage;
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
    void Add(TArg Key)
    {
        void* pValue;
        void* pKey;
        bool bExisted;
        if (core.Add(&Key, true, pKey, pValue, &bExisted))
        {
            if (bExisted)
                Destructor((TStorage*)pKey);
            Constructor((TStorage*)pKey, Key);
        }
    }

    template <typename TColl>
	void AddMany(const TColl& coll)
	{
		for (auto iter = coll.Iterate(); iter.Next(); )
		{
			Add(iter.Get());
		}
	}


    // Remove an item from the Set
    bool Remove(TArg Key)
    {
        void* pOldKey;
        void* pOldValue;
        if (core.Remove(&Key, pOldKey, pOldValue))
        {
            Destructor((TStorage*)pOldKey);
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
                Destructor((TStorage*)pKey);
            }
        }
        core.Clear();
    }

    class Iter
    {
    public:
        const TArg Get() { return *_key; };

        bool Next() { return _owner->GetNext(*this); }

    private:
        Iter(const Set* owner, bool forward, int version)
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
        }

        const TStorage* _key = nullptr;
        const Set* _owner;
        int _pos = -1;
        int _version = 0;
        bool _forward = false;
        friend class Set;
    };

    Iter Iterate() const
    {
        return Iter(this, true, core.get_table_version());
    }

    Iter IterateReverse() const
    {
        Iter iter(this, false, core.get_table_version());
        iter._pos = core.get_table_count();
        return iter;
    }

    bool GetNext(Iter& iter) const
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
                    iter._key = (TStorage*)key;
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
                    iter._key = (TStorage*)key;
                    return true;
                }
                iter._pos--;
            }
        }
        return false;
    }

    // Check if a Set contains a key
    bool Contains(TArg Key) const
    {
        return core.Find(&Key) != nullptr;
    }

    static Set Union(const Set& a, const Set& b)
    {
        Set r;
        r.AddMany(a);
        r.AddMany(b);
        return r;
    }

    static Set Intersection(const Set& a, const Set& b)
    {
        Set r;
        for (auto iter = a.Iterate(); iter.Next(); )
        {
            if (b.Contains(iter.Get()))
                r.Add(iter.Get());
        }
        return r;
    }

    // Items in `a` that are not in `b`
    static Set Difference(const Set& a, const Set& b)
    {
        Set r;
        for (auto iter = a.Iterate(); iter.Next(); )
        {
            if (!b.Contains(iter.Get()))
                r.Add(iter.Get());
        }
        return r;
    }

    // Implementation
private:
    class Core : public HashCore
    {
    public:
        Core() :
            HashCore(sizeof(TStorage), 0)
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
            return TCompare::Hash(*(const TStorage*)a);
        }
        virtual bool KeyEq(const void* a, const void* b) const override
        {
            return TCompare::AreEqual(*(const TStorage*)a, *(const TStorage*)b);
        }
    };
    Core core;

};




}