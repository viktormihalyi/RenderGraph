#ifndef STATICINIT_HPP
#define STATICINIT_HPP

#include "BuildType.hpp"
#include "Noncopyable.hpp"

#include <functional>


class StaticInit : public Noncopyable {
public:
    StaticInit (const std::function<void ()>& callback)
    {
        callback ();
    }
};


class DebugOnlyStaticInit : public Noncopyable {
public:
    DebugOnlyStaticInit (const std::function<void ()>& callback)
    {
        if constexpr (IsDebugBuild) {
            callback ();
        }
    }
};

#endif