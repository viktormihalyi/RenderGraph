#include "Time.hpp"

namespace GVK {

const std::chrono::time_point<TimePoint::Clock> TimePoint::ApplicationStartTime (TimePoint::Clock::now ());

}
