#pragma once

#include <chrono>

// this is tiny so i'll keep its implementation as a header for now
class Timer {
public:
    using Clock = std::chrono::steady_clock;
    using TimePoint = Clock::time_point;

    Timer() : last_update(Clock::now()) {}

    std::chrono::nanoseconds time_elapsed(TimePoint now) const {
        return std::chrono::duration_cast<std::chrono::nanoseconds>(now - last_update);
    }

    bool has_elapsed(TimePoint now, std::chrono::nanoseconds duration) const {
        return time_elapsed(now) > duration;
    }

    void update(TimePoint now) {
        last_update = now;
    }

private:
    TimePoint last_update;
};
