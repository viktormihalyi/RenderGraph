#ifndef MOVABLEPTR_HPP
#define MOVABLEPTR_HPP

#include <type_traits>
#include <utility>


namespace GVK {

template<typename T>
class MovablePtr {
private:
    static_assert (std::is_pointer<T>::value, "T must be a pointer.");

    T ptr;

public:
    MovablePtr ()
        : ptr { nullptr }
    {
    }

    MovablePtr (T ptr_)
        : ptr { ptr_ }
    {
    }

    MovablePtr (const MovablePtr&) = delete;
    MovablePtr& operator= (const MovablePtr&) = delete;

    MovablePtr (MovablePtr&& other) noexcept
        : ptr { other.ptr }
    {
        other.ptr = nullptr;
    }

    MovablePtr& operator= (MovablePtr&& other) noexcept
    {
        ptr       = other.ptr;
        other.ptr = nullptr;
        return *this;
    }

    operator T () { return ptr; }
    operator T () const { return ptr; }

    T Get () { return ptr; }
    T Get () const { return ptr; }

    T operator* () { return ptr; }
    T operator* () const { return ptr; }

    T*       operator& () { return &ptr; }
    const T* operator& () const { return &ptr; }
};

} // namespace GVK


#endif