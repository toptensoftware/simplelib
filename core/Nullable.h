#pragma once

#include <assert.h>

namespace SimpleLib
{

template <typename T>
class Nullable
{
public:
    Nullable()
    {
    }
    Nullable(const T& value) : m_isNull(false), m_value(value)
    {
    }
    Nullable(const Nullable<T>& other) = default;

    T GetValue() const
    {
        assert(!m_isNull);
        return m_value;
    }

    bool HasValue() const
    {
        return !m_isNull;
    }

    Nullable& operator=(const T& value)
    {
        m_value = value;
        m_isNull = false;
        return *this;
    }

    Nullable& operator=(const Nullable<T>& other) = default;

    void Clear()
    {
        m_isNull = true;
        m_value = T();
    }

protected:
    bool m_isNull = true;
    T m_value = T();
};

}