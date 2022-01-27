#ifndef CACHE_HPP
#define CACHE_HPP

#include <functional>

namespace GVK {

template<typename T>
struct Lazy {
private:
    T                   value;
    bool                isValid;
    std::function<T ()> calculator;

public:
    Lazy (const std::function<T ()>& calculator)
        : value ()
        , isValid (false)
        , calculator (calculator)
    {
    }

    void Calculate ()
    {
        value   = calculator ();
        isValid = true;
    }

    inline void Invalidate ()
    {
        isValid = false;
    }

    inline const T& Get ()
    {
        if (!isValid) {
            Calculate ();
        }
        return value;
    }

    inline const T& operator* () { return Get (); }

    inline operator const T& () { return Get (); }
};

} // namespace GVK

#endif