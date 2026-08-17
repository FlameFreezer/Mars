#include <timer/mars_timer.h>
#include <chrono>

namespace mars {
    void PreciseTimer::startInternal(std::unique_lock<std::mutex>&& l) noexcept {
        if(mStatus == TimerStatus::running) return;
        mStatus = TimerStatus::running;
        mTimeLeft = mWaitTime;
        if(mThread.joinable()) mThread.join();
        mThread = std::thread([this]{
            std::this_thread::sleep_for(mWaitTime);
            std::unique_lock<std::mutex> l(mtx);
            //Check if this thread still represents the active timer (i.e. timer wasn't stopped 
            // or paused)
            if(std::this_thread::get_id() == mThread.get_id()) {
                mStatus = TimerStatus::stopped;
                mTimeLeft = std::chrono::steady_clock::duration{0};
            }
        });
        mStartTime = std::chrono::steady_clock::now();
    }
    PreciseTimer::PreciseTimer(float waitTimeS) noexcept {
        const std::chrono::duration<float, std::chrono::seconds::period> s(waitTimeS);
        mWaitTime = std::chrono::duration_cast<std::chrono::steady_clock::duration>(s);
        mTimeLeft = mWaitTime;
    }
    PreciseTimer::~PreciseTimer() noexcept {
        if(mThread.joinable()) mThread.detach();
    }
    void PreciseTimer::start() noexcept {
        std::unique_lock<std::mutex> l(mtx);
        startInternal(std::move(l));
    }
    void PreciseTimer::start(float waitTimeS) noexcept {
        const std::chrono::duration<float, std::chrono::seconds::period> s(waitTimeS);
        std::unique_lock<std::mutex> l(mtx);
        if(mStatus == TimerStatus::running) return;
        mWaitTime = std::chrono::duration_cast<std::chrono::steady_clock::duration>(s);
        mTimeLeft = mWaitTime;
        startInternal(std::move(l));
    }
    void PreciseTimer::stop() noexcept {
        mThread.detach();
        std::unique_lock<std::mutex> l(mtx);
        mStatus = TimerStatus::stopped;
        mTimeLeft = std::chrono::steady_clock::duration{0};
    }
    void PreciseTimer::pause() noexcept {
        mThread.detach();
        std::unique_lock<std::mutex> l(mtx);
        //If the timer was stopped by the thread before we could pause it, do nothing
        if(mStatus == TimerStatus::stopped) return;
        mStatus = TimerStatus::paused;
        mTimeLeft = std::chrono::steady_clock::now() - mStartTime;
    }
    void PreciseTimer::resume() noexcept {
        std::unique_lock<std::mutex> l(mtx);
        if((mStatus != TimerStatus::paused)) return;
        mStatus = TimerStatus::running;
        mThread = std::thread([this]{
            std::this_thread::sleep_for(mTimeLeft);
            std::unique_lock<std::mutex> l(mtx);
            //Check if this thread still represents the active timer (i.e. timer wasn't stopped 
            // or paused)
            if(std::this_thread::get_id() == mThread.get_id()) {
                mStatus = TimerStatus::stopped;
                mTimeLeft = std::chrono::steady_clock::duration{0};
            }
        });
    }
    void PreciseTimer::reset() noexcept {
        std::unique_lock<std::mutex> l(mtx);
        if(mStatus == TimerStatus::running) return;
        mStatus = TimerStatus::stopped;
        mTimeLeft = mWaitTime;
    }
    TimerStatus PreciseTimer::status() const noexcept {
        return mStatus;
    }
    float PreciseTimer::timeLeft() const noexcept {
        return std::chrono::duration_cast<std::chrono::duration<float, std::chrono::seconds::period>>(mTimeLeft).count();
    }
    std::chrono::steady_clock::time_point::duration::rep PreciseTimer::timeLeftSystem() const noexcept {
        return mTimeLeft.count();
    }
    void PreciseTimer::setWaitTime(float waitTime) noexcept {
        const std::chrono::duration<float, std::chrono::seconds::period> s(waitTime);
        std::unique_lock<std::mutex> l(mtx);
        mWaitTime = std::chrono::duration_cast<std::chrono::steady_clock::time_point::duration>(s);
        if(mStatus == TimerStatus::stopped) {
            mTimeLeft = mWaitTime;
        }
    }
    float PreciseTimer::waitTime() const noexcept {
        return std::chrono::duration_cast<std::chrono::duration<float, std::chrono::seconds::period>>(mWaitTime).count();
    }

    Timer::Timer(float waitTimeS) noexcept {
        const std::chrono::duration<float, std::chrono::seconds::period> s{ waitTimeS };
        mWaitTime = std::chrono::duration_cast<std::chrono::steady_clock::duration>(s);
        mTimeLeft = mWaitTime;
    }
    Timer::Timer(const Timer& other) noexcept :  mWaitTime(other.mWaitTime), mTimeLeft(other.mTimeLeft), mStatus(other.mStatus) {}
    void Timer::updateInternal(std::chrono::steady_clock::time_point::duration deltaTime) noexcept {
        if (mStatus == TimerStatus::running) {
            if (mTimeLeft <= deltaTime) {
                mTimeLeft = mTimeLeft.zero();
            }
            else mTimeLeft -= deltaTime;
            if (mTimeLeft == mTimeLeft.zero()) {
                mStatus = TimerStatus::stopped;
            }
        }
    }
    void Timer::update(std::chrono::steady_clock::time_point::duration deltaTime) noexcept {
        updateInternal(deltaTime);
    }
    void Timer::update(float deltaTimeS) noexcept {
        const std::chrono::duration<float, std::chrono::seconds::period> deltaS{ deltaTimeS };
        const std::chrono::steady_clock::time_point::duration deltaTime = std::chrono::duration_cast<std::chrono::steady_clock::duration>(deltaS);
        updateInternal(deltaTime);
    }
    void Timer::startInternal() noexcept {
        mStatus = TimerStatus::running;
        mTimeLeft = mWaitTime;
    }
    void Timer::start() noexcept {
        if (mStatus == TimerStatus::running) return;
        startInternal();
    }
    void Timer::start(float waitTimeS) noexcept {
        const std::chrono::duration<float, std::chrono::seconds::period> waitS{ waitTimeS };
        const std::chrono::steady_clock::duration waitTime = std::chrono::duration_cast<std::chrono::steady_clock::duration>(waitS);

        if (mStatus == TimerStatus::running) return;
        mWaitTime = waitTime;
        startInternal();
    }
    void Timer::stop() noexcept {
        mStatus = TimerStatus::stopped;
        mTimeLeft = mTimeLeft.zero();
    }
    void Timer::pause() noexcept {
        if (mStatus == TimerStatus::stopped) return;
        mStatus = TimerStatus::paused;
    }
    void Timer::resume() noexcept {
        if (mStatus == TimerStatus::running) return;
        mStatus = TimerStatus::running;
    }
    void Timer::reset() noexcept {
        if (mStatus == TimerStatus::running) return;
        mTimeLeft = mWaitTime;
        mStatus = TimerStatus::stopped;
    }
    TimerStatus Timer::status() const noexcept {
        return mStatus;
    }
    float Timer::timeLeft() const noexcept {
        const std::chrono::duration<float, std::chrono::seconds::period> timeLeft{ mTimeLeft };
        return timeLeft.count();
    }
    std::chrono::steady_clock::time_point::duration::rep Timer::timeLeftSystem() const noexcept {
        return mTimeLeft.count();
    }
    void Timer::setWaitTime(float waitTimeS) noexcept {
        const std::chrono::duration<float, std::chrono::seconds::period> s{waitTimeS};
        mWaitTime = std::chrono::duration_cast<std::chrono::steady_clock::time_point::duration>(s);
        if(mStatus == TimerStatus::stopped) {
            mTimeLeft = mWaitTime;
        }
    }
    float Timer::waitTime() const noexcept {
        const std::chrono::duration<float, std::chrono::seconds::period> waitTimeS{ mWaitTime };
        return waitTimeS.count();
    }
}