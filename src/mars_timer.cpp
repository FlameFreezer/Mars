#include "mars_timer.h"
#include <chrono>

namespace mars {
    void Timer::startInternal(std::unique_lock<std::mutex>&& l) noexcept {
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
    Timer::Timer(float waitTimeS) {
        const std::chrono::duration<float, std::chrono::seconds::period> s(waitTimeS);
        mTime = std::chrono::duration_cast<std::chrono::steady_clock::duration>(s);
        mTimeLeft = mTime;
    }
    Timer::~Timer() noexcept {
        if(mThread.joinable()) mThread.detach();
    }
    void Timer::start() noexcept {
        std::unique_lock<std::mutex> l(mtx);
        startInternal(std::move(l));
    }
    void Timer::start(float waitTimeS) noexcept {
        const std::chrono::duration<float, std::chrono::seconds::period> s(waitTimeS);
        std::unique_lock<std::mutex> l(mtx);
        if(mStatus == TimerStatus::running) return;
        mTime = std::chrono::duration_cast<std::chrono::steady_clock::duration>(s);
        mTimeLeft = mTime;
        startInternal(std::move(l));
    }
    void Timer::stop() noexcept {
        mThread.detach();
        std::unique_lock<std::mutex> l(mtx);
        mStatus = TimerStatus::stopped;
        mTimeLeft = std::chrono::steady_clock::duration{0};
    }
    void Timer::pause() noexcept {
        mThread.detach();
        std::unique_lock<std::mutex> l(mtx);
        //If the timer was stopped by the thread before we could pause it, do nothing
        if(mStatus == TimerStatus::stopped) return;
        mStatus = TimerStatus::paused;
        mTimeLeft = std::chrono::steady_clock::now() - mStartTime;
    }
    void Timer::resume() noexcept {
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
    void Timer::reset() noexcept {
        std::unique_lock<std::mutex> l(mtx);
        if(mStatus == TimerStatus::running) return;
        mStatus = TimerStatus::stopped;
        mTimeLeft = mTime;
    }
    TimerStatus Timer::status() const noexcept {
        return mStatus;
    }
    float Timer::timeLeft() const noexcept {
        return std::chrono::duration_cast<std::chrono::duration<float, std::chrono::seconds::period>>(mTimeLeft).count();
    }
    std::chrono::steady_clock::time_point::duration::rep Timer::timeLeftSystem() const noexcept {
        return mTimeLeft.count();
    }
    void Timer::setWaitTime(float waitTime) noexcept {
        const std::chrono::duration<float, std::chrono::seconds::period> s(waitTime);
        std::unique_lock<std::mutex> l(mtx);
        mTime = std::chrono::duration_cast<std::chrono::steady_clock::time_point::duration>(s);
        if(mStatus == TimerStatus::stopped) {
            mTimeLeft = mTime;
        }
    }
    float Timer::waitTime() const noexcept {
        return std::chrono::duration_cast<std::chrono::duration<float, std::chrono::seconds::period>>(mTime).count();
    }

}