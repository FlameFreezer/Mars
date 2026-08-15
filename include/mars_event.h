#pragma once

#include <limits>

#include "mars_swaparray.h"

namespace mars {
    template<class... Args> class Event;

    template<class... Args>
    class IObserver {
    public:
        friend class Event<Args...>;
        virtual ~IObserver() noexcept {
            if (mEvent) {
                mEvent->detach(*this);
            }
        }
        bool isAttached() const noexcept {
            return mEvent != nullptr;
        }
    private:
        virtual void invoke(Args...) noexcept = 0;
        Event<Args...>* mEvent = nullptr;
        size_t mObserverIndex = std::numeric_limits<size_t>::max();
    };

    template<class Owner, class... Args>
    class Observer : public IObserver<Args...> {
    public:
        friend class Event<Args...>;
        Observer(Owner& owner) noexcept : mOwner(owner), mMethod(nullptr) {}
    private:
        virtual void invoke(Args... args) noexcept override {
            if (mMethod) {
                (mOwner.*mMethod)(args...);
            }
        }
        Owner& mOwner;
        void (Owner::*mMethod)(Args...);
    };

    template<class... Args>
    class Event {
    public:
        Event() noexcept = default;
        Event(const Event&) = delete;
        Event(Event&& other) noexcept : mObservers(std::move(other.mObservers)) {}
        ~Event() noexcept {
            while(mObservers.size() != 0) {
                detach(*mObservers[0]);
            }
        }
        Event& operator=(const Event&) = delete;
        Event& operator=(Event&& other) noexcept {
            if (this == &other) return *this;
            mObservers = std::move(other.mObservers);
            return *this;
        }
        void invoke(Args... args) const noexcept {
            for (size_t i = 0; i < mObservers.size(); i++) {
                mObservers[i]->invoke(args...);
            }
        }
        template<class Owner>
        void attach(Observer<Owner, Args...>& observer, void (Owner::*method)(Args...)) noexcept {
            observer.mMethod = method;
            observer.mEvent = this;
            observer.mObserverIndex = mObservers.size();
            mObservers.pushBack(&observer);
        }
        void detach(IObserver<Args...>& observer) {
            mObservers.erase(observer.mObserverIndex);
            if (mObservers.size() > 0) {
                mObservers[observer.mObserverIndex]->mObserverIndex = observer.mObserverIndex;
            }
            observer.mEvent = nullptr;
            observer.mObserverIndex = std::numeric_limits<size_t>::max();
        }
        size_t getObserverCount() const noexcept {
            return mObservers.size();
        }
    private:
        SwapArray<IObserver<Args...>*> mObservers;
    };
}