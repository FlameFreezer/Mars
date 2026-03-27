module;

#include <thread>
#include <chrono>
#include <mutex>

export module timer;
import types;

namespace mars {
    export enum class TimerStatus : u8 {
        running,
        paused,
        stopped
    };

    export class Timer {
        std::mutex mtx;
        TimerStatus mStatus = TimerStatus::stopped;
        std::thread mThread;
        std::chrono::nanoseconds mTime = std::chrono::nanoseconds(0);
        std::chrono::nanoseconds mTimeLeft = std::chrono::nanoseconds(0);
        std::chrono::steady_clock::time_point mStartTime;
        public:
        Timer() = default;
        Timer(float waitTimeS) {
            const std::chrono::duration<float, std::chrono::seconds::period> s(waitTimeS);
            mTime = std::chrono::duration_cast<std::chrono::nanoseconds>(s);
            mTimeLeft = mTime;
        }
        ~Timer() noexcept {
            if(mThread.joinable()) mThread.join();
        }
        /// Starts the timer from time t = 0
        /// Postconditions:     status() == TimerStatus::running
        /// Returns: void
        void start() noexcept {
            std::unique_lock<std::mutex> l(mtx);
            mStatus = TimerStatus::running;
            mTimeLeft = mTime;
            l.unlock();
            if(mThread.joinable()) mThread.join();
            mThread = std::thread([this]{
                std::this_thread::sleep_for(mTime);
                std::unique_lock<std::mutex> l(mtx);
                if(mStatus != TimerStatus::paused) {
                    mStatus = TimerStatus::stopped;
                    mTimeLeft = std::chrono::nanoseconds(0);
                }
            });
            mStartTime = std::chrono::steady_clock::now();
        }
        /// Stops the timer, leaving it in the stopped status with no time left
        /// Postconditions: timeLeft() == 0.0f
        ///                 status() == TimerStatus::stopped 
        /// Returns: void
        void stop() noexcept {
            mThread.detach();
            std::unique_lock<std::mutex> l(mtx);
            //Do nothing if the timer is already stopped
            if(mStatus == TimerStatus::stopped) return;
            mStatus = TimerStatus::stopped;
            mTimeLeft = std::chrono::nanoseconds(0);
        }
        /// Pauses the timer, leaving it in the paused status
        /// Postconditions: status() == TimerStatus::paused
        /// Returns: void
        void pause() noexcept {
            mThread.detach();
            std::unique_lock<std::mutex> l(mtx);
            //If the timer was stopped by the thread before we could pause it, do nothing
            if(mStatus == TimerStatus::stopped) return;
            mStatus = TimerStatus::paused;
            mTimeLeft = std::chrono::steady_clock::now() - mStartTime;
        }
        /// Resumes the timer, waiting for however much time was left since pausing. Does nothing if the timer was not paused. Does not modify the return value of waitTime().
        /// Preconditions:  status() == TimerStatus::paused
        /// Postconditions: status() == TimerStatus::running
        /// Returns: void
        void resume() noexcept {
            std::unique_lock<std::mutex> l(mtx);
            if((mStatus != TimerStatus::paused)) return;
            mStatus = TimerStatus::running;
            l.unlock();
            mThread = std::thread([this]{
                std::this_thread::sleep_for(mTimeLeft);
                std::unique_lock<std::mutex> l(mtx);
                if(mStatus != TimerStatus::paused) {
                    mStatus = TimerStatus::stopped;
                    mTimeLeft = std::chrono::nanoseconds(0);
                }
            });
        }
        /// Resets the timer to its initial conditions. Does nothing if the timer is running
        /// Preconditions:  status() != TimerStatus::running
        /// Postconditions: status() == TimerStatus::stopped
        ///                 timeLeft() == waitTime()
        /// Returns: void
        void reset() noexcept {
            std::unique_lock<std::mutex> l(mtx);
            if(mStatus == TimerStatus::running) return;
            mStatus = TimerStatus::stopped;
            mTimeLeft = mTime;
        }
        TimerStatus status() const noexcept {
            return mStatus;
        }
        float timeLeft() const noexcept {
            return std::chrono::duration_cast<std::chrono::duration<float, std::chrono::seconds::period>>(mTimeLeft).count();
        }
        std::chrono::nanoseconds::rep const timeLeftNanoseconds() noexcept {
            return mTimeLeft.count();
        }
        /// Sets the total wait time to the number of seconds passed. If the timer was paused, this will NOT change the time left on the timer.
        /// Arguments:  waitTime    The amount of time the timer should wait for when start() is called
        /// Returns: void
        void setWaitTime(float waitTime) noexcept {
            const std::chrono::duration<float, std::chrono::seconds::period> s(waitTime);
            mTime = std::chrono::duration_cast<std::chrono::nanoseconds>(s);
            if(mStatus == TimerStatus::stopped) {
                mTimeLeft = mTime;
            }
        }
        float waitTime() const noexcept {
            return std::chrono::duration_cast<std::chrono::duration<float, std::chrono::seconds::period>>(mTime).count();
        }
    };
}