#pragma once

#include <type_traits>
#include <utility>

template <typename Signature, size_t StorageSize = 0>
class Delegate;

// Inline storage: never allocates. Callable must fit in StorageSize bytes
// (checked at compile time). Use this specialization (StorageSize > 0) on
// for cases where memory allocations can't be tolerated.
template <typename R, typename... Args, size_t StorageSize>
class Delegate<R(Args...), StorageSize>
{
public:
    Delegate() = default;
    Delegate(std::nullptr_t) {}

    Delegate& operator=(std::nullptr_t)
    {
        destroy();
        return *this;
    }

    template <typename F, typename = std::enable_if_t<!std::is_same_v<std::decay_t<F>, std::nullptr_t> &&
                                                        !std::is_same_v<std::decay_t<F>, Delegate>>>
    Delegate(F f)
    {
        static_assert(sizeof(F) <= sizeof(m_storage), "Callable too large for inline storage");
        static_assert(alignof(F) <= alignof(std::max_align_t), "Callable alignment too strict");

        new (&m_storage) F(std::move(f));

        m_invoke = [](void* storage, Args... args) -> R {
            return (*reinterpret_cast<F*>(storage))(std::forward<Args>(args)...);
            };
        m_destroy = [](void* storage) {
            reinterpret_cast<F*>(storage)->~F();
            };
        m_copy = [](void* dst, const void* src) {
            new (dst) F(*reinterpret_cast<const F*>(src));
            };
        m_move = [](void* dst, void* src) {
            new (dst) F(std::move(*reinterpret_cast<F*>(src)));
            reinterpret_cast<F*>(src)->~F();
            };
    }

    Delegate(const Delegate& other)
    {
        copyFrom(other);
    }

    Delegate& operator=(const Delegate& other)
    {
        if (this != &other)
        {
            destroy();
            copyFrom(other);
        }
        return *this;
    }

    Delegate(Delegate&& other) noexcept
    {
        moveFrom(other);
    }

    Delegate& operator=(Delegate&& other) noexcept
    {
        if (this != &other)
        {
            destroy();
            moveFrom(other);
        }
        return *this;
    }

    ~Delegate()
    {
        destroy();
    }

    R operator()(Args... args) const
    {
        return m_invoke(const_cast<void*>(static_cast<const void*>(&m_storage)), std::forward<Args>(args)...);
    }

    bool valid() const { return m_invoke != nullptr; }
    explicit operator bool() const { return valid(); }

private:
    void destroy()
    {
        if (m_destroy)
            m_destroy(&m_storage);
        m_invoke = nullptr;
        m_destroy = nullptr;
        m_copy = nullptr;
        m_move = nullptr;
    }

    void copyFrom(const Delegate& other)
    {
        m_invoke = other.m_invoke;
        m_destroy = other.m_destroy;
        m_copy = other.m_copy;
        m_move = other.m_move;
        if (m_copy)
            m_copy(&m_storage, &other.m_storage);
    }

    // Move-constructs the callable out of other's storage into ours (rather
    // than copying then discarding the source), so move-only captures work
    // and moves of non-trivial captures aren't a wasted copy.
    void moveFrom(Delegate& other)
    {
        m_invoke = other.m_invoke;
        m_destroy = other.m_destroy;
        m_copy = other.m_copy;
        m_move = other.m_move;
        if (m_move)
            m_move(&m_storage, &other.m_storage);

        // other's callable was already moved-from and destroyed by m_move
        // above, so just clear its pointers - not a call to other.destroy(),
        // which would try to destroy it a second time.
        other.m_invoke = nullptr;
        other.m_destroy = nullptr;
        other.m_copy = nullptr;
        other.m_move = nullptr;
    }

    alignas(std::max_align_t) unsigned char m_storage[StorageSize];
    R(*m_invoke)(void*, Args...) = nullptr;
    void (*m_destroy)(void*) = nullptr;
    void (*m_copy)(void*, const void*) = nullptr;
    void (*m_move)(void*, void*) = nullptr;
};

// StorageSize == 0 (the default): heap-allocates the callable. Same
// interface as the inline version but with no size limit, at the cost of an
// allocation on construction/copy. Not for use on real-time threads - use
// Delegate<Signature, N> there instead.
template <typename R, typename... Args>
class Delegate<R(Args...), 0>
{
public:
    Delegate() = default;
    Delegate(std::nullptr_t) {}

    Delegate& operator=(std::nullptr_t)
    {
        destroy();
        return *this;
    }

    template <typename F, typename = std::enable_if_t<!std::is_same_v<std::decay_t<F>, std::nullptr_t> &&
                                                        !std::is_same_v<std::decay_t<F>, Delegate>>>
    Delegate(F f)
    {
        m_storage = new F(std::move(f));

        m_invoke = [](void* storage, Args... args) -> R {
            return (*reinterpret_cast<F*>(storage))(std::forward<Args>(args)...);
            };
        m_destroy = [](void* storage) {
            delete reinterpret_cast<F*>(storage);
            };
        m_copy = [](const void* src) -> void* {
            return new F(*reinterpret_cast<const F*>(src));
            };
    }

    Delegate(const Delegate& other)
    {
        copyFrom(other);
    }

    Delegate& operator=(const Delegate& other)
    {
        if (this != &other)
        {
            destroy();
            copyFrom(other);
        }
        return *this;
    }

    Delegate(Delegate&& other) noexcept
    {
        moveFrom(other);
    }

    Delegate& operator=(Delegate&& other) noexcept
    {
        if (this != &other)
        {
            destroy();
            moveFrom(other);
        }
        return *this;
    }

    ~Delegate()
    {
        destroy();
    }

    R operator()(Args... args) const
    {
        return m_invoke(m_storage, std::forward<Args>(args)...);
    }

    bool valid() const { return m_invoke != nullptr; }
    explicit operator bool() const { return valid(); }

private:
    void destroy()
    {
        if (m_destroy && m_storage)
            m_destroy(m_storage);
        m_storage = nullptr;
        m_invoke = nullptr;
        m_destroy = nullptr;
        m_copy = nullptr;
    }

    void copyFrom(const Delegate& other)
    {
        m_invoke = other.m_invoke;
        m_destroy = other.m_destroy;
        m_copy = other.m_copy;
        m_storage = m_copy ? m_copy(other.m_storage) : nullptr;
    }

    // Move is just a pointer steal - no allocation and no dependency on F
    // having a move constructor.
    void moveFrom(Delegate& other)
    {
        m_invoke = other.m_invoke;
        m_destroy = other.m_destroy;
        m_copy = other.m_copy;
        m_storage = other.m_storage;

        other.m_invoke = nullptr;
        other.m_destroy = nullptr;
        other.m_copy = nullptr;
        other.m_storage = nullptr;
    }

    void* m_storage = nullptr;
    R(*m_invoke)(void*, Args...) = nullptr;
    void (*m_destroy)(void*) = nullptr;
    void* (*m_copy)(const void*) = nullptr;
};
