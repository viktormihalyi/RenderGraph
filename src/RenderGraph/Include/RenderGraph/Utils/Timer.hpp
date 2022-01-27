#ifndef UTILS_TIMER
#define UTILS_TIMER

#include "BuildType.hpp"

#include <chrono>
#include <string>

namespace Utils {

class TimerObserver {
public:
    using Duration = std::chrono::duration<double>;

    virtual void TimerEnded (Duration delta) = 0;
};


class TimerScope {
private:
    TimerObserver&                                              observer;
    std::chrono::time_point<std::chrono::high_resolution_clock> startTime;

public:
    TimerScope (TimerObserver& observer)
        : observer (observer)
        , startTime (std::chrono::high_resolution_clock::now ())
    {
    }

    ~TimerScope ()
    {
        observer.TimerEnded (std::chrono::high_resolution_clock::now () - startTime);
    }
};


class DummyTimerLogger final : public TimerObserver {
private:
    void TimerEnded (Duration) override
    {
    }
};


} // namespace Utils

#endif