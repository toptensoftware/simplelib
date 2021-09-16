#ifndef __simplelib_dyntype_h__
#define __simplelib_dyntype_h__


#include <assert.h>
#include <string.h>

namespace SimpleLib
{
	// WHAT THE HELL IS THIS???
	// It's a cross platform version of __declspec(selectany) to store
	// the global pointer to the first type entry
	class CDynType;
	template <int iDummy = 0>
	struct CDynTypeFirstHolder
	{
		static CDynType* m_pFirst;
	};
	template <int iDummy> CDynType* CDynTypeFirstHolder<iDummy>::m_pFirst = nullptr;

	// Store information about a CDynamicBase type
	class CDynType
	{
	public:
		// Construction
		CDynType(int iID, void* (*pfnCreate)(), const char* pszName) :
			m_iID(iID),
			m_pfnCreate(pfnCreate),
			m_pszName(pszName)
		{
			// Check for duplicate type id's
			assert(iID == 0 || GetTypeFromID(iID) == nullptr);

			m_pNext = CDynTypeFirstHolder<>::m_pFirst;
			CDynTypeFirstHolder<>::m_pFirst = this;
		}

		// Given a type ID, return the CDynType for it
		static CDynType* GetTypeFromID(int iID)
		{
			CDynType* p = CDynTypeFirstHolder<>::m_pFirst;
			while (p)
			{
				if (p->m_iID == iID)
					return p;
				p = p->m_pNext;
			}
			return nullptr;
		}

		static CDynType* GetTypeFromName(const char* pszName)
		{
			CDynType* p = CDynTypeFirstHolder<>::m_pFirst;
			while (p)
			{
				if (p->m_pszName != nullptr)
				{
					if (p->m_pszName == pszName || strcmp(p->m_pszName, pszName) == 0)
						return p;
				}
				p = p->m_pNext;
			}
			return nullptr;
		}

		void* CreateInstance() const
		{
			assert(m_pfnCreate != nullptr);
			return m_pfnCreate();
		}

		int GetID() const
		{
			return m_iID;
		}

		const char* GetName() const
		{
			return m_pszName;
		}

	protected:
		int	m_iID;
		void* (*m_pfnCreate)();
		const char* m_pszName;
		CDynType* m_pNext;
	};

	// Base class for CDynamicBase classes that dont derive from another dynamic type
	class CDynamicBaseNone
	{
	public:
		virtual void* QueryCast(CDynType* ptype) { return nullptr; }
		virtual CDynType* QueryType() { return nullptr; };
		static CDynType* GetType() { return nullptr; };
	};

	// CDynamicBase
	template <typename TSelf, typename TBase = CDynamicBaseNone>
	class CDynamicBase : public TBase
	{
	public:
		template <typename T>
		T* As()
		{
			if (!this) return nullptr;
			T* p = QueryAs<T>();
			assert(p != nullptr);
			return p;
		}

		template <typename T>
		T* QueryAs()
		{
			return (T*)QueryCast(&T::dyntype);
		}

		static CDynType* GetType()
		{
			return &TSelf::dyntype;
		}

		static CDynType* GetBaseType()
		{
			return TBase::GetType();
		}

		static const char* GetTypeName()
		{
			return nullptr;
		}

		virtual void* QueryCast(CDynType* ptype)
		{
			if (ptype == GetType())
			{
				return static_cast<TSelf*>(this);
			}

			void* p = TBase::QueryCast(ptype);
			return p;
		}

		virtual CDynType* QueryType()
		{
			return GetType();
		}
	};


	// CDynamic
	template <typename TSelf, typename TBase = CDynamicBaseNone>
	class CDynamic : public CDynamicBase<TSelf, TBase>
	{
	public:
		static CDynType dyntype;
	};


	// CDynamicCreatable
	template <typename TSelf, typename TBase = CDynamicBaseNone, int iID = 0>
	class CDynamicCreatable : public CDynamicBase<TSelf, TBase>
	{
	public:
		static void* CreateInstance()
		{
			return new TSelf();
		}

		static int GenerateTypeID()
		{
			return 0;
		}
		static CDynType dyntype;
	};

	template <typename TSelf, typename TBase>
	CDynType CDynamic<TSelf, TBase>::dyntype(0, nullptr, TSelf::GetTypeName());

	template <typename TSelf, typename TBase, int iID>
	CDynType CDynamicCreatable<TSelf, TBase, iID>::dyntype(iID ? iID : TSelf::GenerateTypeID(), TSelf::CreateInstance, TSelf::GetTypeName());

} // namespace

#endif  // __simplelib_dyntype_h__
