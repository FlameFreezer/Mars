module;

#include <chrono>

export module time;

namespace mars {
    export class Time {
        std::chrono::steady_clock::time_point mTime = std::chrono::steady_clock::now();
        std::chrono::nanoseconds mDelta = std::chrono::nanoseconds(0);
        Time() noexcept = default;
        Time(const Time& other) = delete;
        Time(Time&& other) = delete;
        public:
        static Time& get() noexcept;
        /// Gets the epoch time for the start of the frame. Make sure to call `updateTime` before any calls to this within the current frame.
        /// Returns:     The epoch time of the start of the current frame, in nanoseconds
        std::chrono::steady_clock::time_point frameTime() const noexcept;
        /// Gets the time between the last frame and the current frame. Make sure to call `updateTime` before any calls to this within the current frame.
        /// Returns:     The time between the last frame and the current frame, in nanoseconds
        std::chrono::nanoseconds::rep deltaNanos() const noexcept;
        /// Gets the time between the last frame and the current frame. Make sure to call `updateTime` before any calls to this within the current frame.
        /// Returns: float   The time between the last frame and the current frame, in seconds
        float delta() const noexcept;
        /// Updates the `time` and `deltaTime` private class members to reflect the current frame. Should be called at the start of the current frame.
        /// Returns: void    Nothing
        void update() noexcept;
    };
}
