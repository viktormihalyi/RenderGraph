#ifndef TIMEPOINT_HPP
#define TIMEPOINT_HPP

#include "RenderGraph/RenderGraphExport.hpp"

#include <chrono>

namespace GVK {

class TimePoint {
public:
    using Precision = std::chrono::nanoseconds;
    using Clock     = std::chrono::high_resolution_clock;

private:
    uint64_t nanoseconds;
#ifndef NDEBUG
    double seconds;
    double milliseconds;
    double microseconds;
#endif


public:
    RENDERGRAPH_DLL_EXPORT TimePoint (uint64_t nanoseconds = 0)
        : nanoseconds (nanoseconds)
#ifndef NDEBUG
        , seconds (AsSeconds ())
        , milliseconds (AsMilliseconds ())
        , microseconds (AsMicroseconds ())
#endif
    {
    }

    double RENDERGRAPH_DLL_EXPORT AsSeconds () const { return static_cast<double> (nanoseconds * 1.0e-9); }
    double RENDERGRAPH_DLL_EXPORT AsMilliseconds () const { return static_cast<double> (nanoseconds * 1.0e-6); }
    double RENDERGRAPH_DLL_EXPORT AsMicroseconds () const { return static_cast<double> (nanoseconds * 1.0e-3); }
    double RENDERGRAPH_DLL_EXPORT AsNanoseconds () const { return static_cast<double> (nanoseconds); }

    RENDERGRAPH_DLL_EXPORT operator uint64_t () const { return nanoseconds; }

    TimePoint RENDERGRAPH_DLL_EXPORT operator- (TimePoint other) const
    {
        return TimePoint (nanoseconds - other.nanoseconds);
    }

    static TimePoint RENDERGRAPH_DLL_EXPORT SinceEpoch ()
    {
        return std::chrono::duration_cast<Precision> (Clock::now ().time_since_epoch ()).count ();
    }

    static TimePoint RENDERGRAPH_DLL_EXPORT SinceApplicationStart ()
    {
        return std::chrono::duration_cast<Precision> (Clock::now () - ApplicationStartTime).count ();
    }

    static const std::chrono::time_point<TimePoint::Clock> RENDERGRAPH_DLL_EXPORT ApplicationStartTime;
};

using TimeDelta = TimePoint;

} // namespace GVK

#endif