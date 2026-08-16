#pragma once

#include <functional>

#include "mars_swaparray.h"

namespace mars {
    template<class... Args> class Event;
    template<class... Args> class IObserver;

    // Handles the bi-directionality of Events and Observers. Observers need to detach themselves 
    // when they go out of scope, and Events need to detach their observers when they go out of 
    // scope. This struct allows both objects to alert the other of their destruction.
    template<class... Args>
    struct ObserverMeta {
        Event<Args...>* event = nullptr;
        IObserver<Args...>* observer = nullptr;
    };
    template<class... Args>
    class IObserver {
    public:
        IObserver() noexcept {
            mMeta->observer = this;
        }
        IObserver(IObserver&& other) noexcept : mMeta(other.mMeta)  {
            mMeta->observer = this;
            other.mMeta = nullptr;
        }
        IObserver& operator=(IObserver&& other) noexcept {
            if (this != &other) {
                mMeta = other.mMeta;
                mMeta->observer = this;

                other.mMeta = nullptr;
            }
            return *this;
        }
        virtual ~IObserver() noexcept {
            // mMeta == nullptr if this is a moved-from object
            if (mMeta) {
                mMeta->observer = nullptr;

                // If we aren't attached to an event, we can delete the metadata.
                // Otherwise, the event is in charge of deleting the metadata; 
                if(mMeta->event == nullptr) delete mMeta;
            }
        }
        bool isAttached() const noexcept {
            return mMeta->event != nullptr;
        }
        void detach() const noexcept {
            mMeta->event = nullptr;
        }
        ObserverMeta<Args...>* getMetadata() noexcept {
            return mMeta;
        }
        virtual void invoke(Args...) noexcept = 0;
    private:
        ObserverMeta<Args...>* mMeta = new ObserverMeta<Args...>;
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
        Event(Event&& other) noexcept : mObservers(std::move(other.mObservers)) {
            foreachObserver([this](ObserverMeta<Args...>* observer){observer->event = this;});
        }
        ~Event() noexcept {
            while(mObservers.size() != 0) {
                // If the observer was already detached, we have to delete the metadata
                if (mObservers[0]->observer == nullptr) {
                    delete mObservers[0];
                }
                // Otherwise, detach the observer
                else {
                    mObservers[0]->observer->detach();
                }
                mObservers.erase(0);
            }
        }
        Event& operator=(const Event&) = delete;
        Event& operator=(Event&& other) noexcept {
            if (this != &other) {
                mObservers = std::move(other.mObservers);
                foreachObserver([this](ObserverMeta<Args...>* observer){observer->event = this;});
            }
            return *this;
        }
        void invoke(Args... args) noexcept {
            foreachObserver([&args...](ObserverMeta<Args...>* observer){observer->observer->invoke(args...);});
        }
        template<class Owner>
        void attach(Observer<Owner, Args...>& observer, void (Owner::*method)(Args...)) noexcept {
            observer.getMetadata()->event = this;
            observer.mMethod = method;
            mObservers.pushBack(observer.getMetadata());
        }
        size_t getObserverCount() const noexcept {
            return mObservers.size();
        }
    private:
        void foreachObserver(std::function<void(ObserverMeta<Args...>*)> function) noexcept {
            for (size_t i = 0; i < mObservers.size(); i++) {
                // If the observer was detached, delete this metadata
                if (mObservers[i]->observer == nullptr) {
                    delete mObservers[i];
                    mObservers.erase(i);
                    i--;
                }
                // Otherwise, perform the provided procedure on the observer
                else {
                    function(mObservers[i]);
                }
            }
        }
        SwapArray<ObserverMeta<Args...>*> mObservers;
    };
}