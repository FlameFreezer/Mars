#pragma once

#include <thread>
#include <chrono>
#include <mutex>

#include "mars_types.h"

namespace mars {
    enum class TimerStatus : u8 {
        running,
        paused,
        stopped
    };

    class Timer {
        std::mutex mtx;
        std::thread mThread;
        std::chrono::steady_clock::time_point::duration mTime {0};
        std::chrono::steady_clock::time_point::duration mTimeLeft {0};
        std::chrono::steady_clock::time_point mStartTime;
        TimerStatus mStatus = TimerStatus::stopped;
        void startInternal(std::unique_lock<std::mutex>&& l) noexcept;
        public:
        Timer() = default;
        Timer(float waitTimeS);
        ~Timer() noexcept;
        /// Starts the timer from time t = 0. Does nothing if the timer is already running.
        /// Preconditions:  status() != TimerStatus::running
        /// Postconditions: status() == TimerStatus::running
        /// Returns: void
        void start() noexcept ;       
        /// Starts the timer from time t = 0 and sets the wait time to the value provided (in seconds). Does nothing if the timer is already running.
        /// Preconditions:  status() != TimerStatus::running
        /// Postconditions: status() == TimerStatus::running
        /// Arguments:      waitTimeS   The amount of time to wait for, in seconds 
        /// Returns: void
        void start(float waitTimeS) noexcept;
        /// Stops the timer, leaving it in the stopped status with no time left.
        /// Postconditions: timeLeft() == 0.0f
        ///                 status() == TimerStatus::stopped 
        /// Returns: void
        void stop() noexcept;
        /// Pauses the timer, leaving it in the paused status. Does nothing if the timer was already stopped.
        /// Preconditions:  status() != TimerStatus::stopped
        /// Postconditions: status() == TimerStatus::paused, if timer was running
        ///                 status() == TimerStatus::stopped, if timer was stopped
        /// Returns: void
        void pause() noexcept;
        /// Resumes the timer, waiting for however much time was left since pausing. Does nothing if the timer was not paused. Does not modify the return value of waitTime().
        /// Preconditions:  status() == TimerStatus::paused
        /// Postconditions: status() == TimerStatus::running
        /// Returns: void
        void resume() noexcept;
        /// Resets the timer to its initial conditions. Does nothing if the timer is running
        /// Preconditions:  status() != TimerStatus::running
        /// Postconditions: status() == TimerStatus::stopped
        ///                 timeLeft() == waitTime()
        /// Returns: void
        void reset() noexcept;
        /// Retrieves the current status of the timer.
        /// Returns: TimerStatus     
        TimerStatus status() const noexcept;
        /// Retrieves the amount of time left on the timer in seconds. Only works if the timer is paused or stopped. Will return innaccurate info while timer is running
        /// Preconditions:  status() != TimerStatus::running
        /// Returns: float
        float timeLeft() const noexcept;
        /// Retrieves the amount of time left on the timer in the system's highest resolution. Only works if the timer is paused or stopped. Will return innaccurate info while timer is running
        /// Preconditions:  status() != TimerStatus::running
        /// Returns: systemTime
        std::chrono::steady_clock::time_point::duration::rep timeLeftSystem() const noexcept;
        /// Sets the total wait time to the number of seconds passed. If the timer was paused, this will NOT change the time left on the timer.
        /// Arguments:  waitTime    The amount of time the timer should wait for when start() is called
        /// Returns: void
        void setWaitTime(float waitTime) noexcept;
        /// Retrieves the total wait time in seconds.
        /// Returns: float
        float waitTime() const noexcept;
    };
}
