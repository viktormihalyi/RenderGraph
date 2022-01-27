#include "Time.hpp"

namespace RG {

const std::chrono::time_point<TimePoint::Clock> TimePoint::ApplicationStartTime (TimePoint::Clock::now ());

}
