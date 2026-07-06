#include "mars_timer.h"
#include <chrono>

namespace mars {
    void PreciseTimer::startInternal(std::unique_lock<std::mutex>&& l) noexcept {
        if(mStatus == TimerStatus::running) return;
        mStatus = TimerStatus::running;
        mTimeLeft = mTime;
        l.unlock();
        if(mThread.joinable()) mThread.join();
        mThread = std::thread([this]{
            std::this_thread::sleep_for(mTime);
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
    PreciseTimer::PreciseTimer(float waitTimeS) {
        const std::chrono::duration<float, std::chrono::seconds::period> s(waitTimeS);
        mTime = std::chrono::duration_cast<std::chrono::steady_clock::duration>(s);
        mTimeLeft = mTime;
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
        mTime = std::chrono::duration_cast<std::chrono::steady_clock::duration>(s);
        mTimeLeft = mTime;
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
        l.unlock();
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
        mTimeLeft = mTime;
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
        mTime = std::chrono::duration_cast<std::chrono::steady_clock::time_point::duration>(s);
        if(mStatus == TimerStatus::stopped) {
            mTimeLeft = mTime;
        }
    }
    float PreciseTimer::waitTime() const noexcept {
        return std::chrono::duration_cast<std::chrono::duration<float, std::chrono::seconds::period>>(mTime).count();
    }

}