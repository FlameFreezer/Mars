module;

#include <chrono>

module time;

namespace mars {
    Time& Time::get() noexcept {
        static Time instance;
        return instance;
    }
    std::chrono::steady_clock::time_point Time::frameTime() const noexcept {
        return mTime;
    }
    std::chrono::nanoseconds::rep Time::deltaNanos() const noexcept {
        return mDelta.count();
    }
    float Time::delta() const noexcept {
        return std::chrono::duration_cast<std::chrono::duration<float, std::chrono::seconds::period>>(mDelta).count();
    }
    void Time::update() noexcept {
        const auto now = std::chrono::steady_clock::now();
        mDelta = now - mTime;
        mTime = now;
    }
}
