#ifndef EVENT_HPP
#define EVENT_HPP

#include "Assert.hpp"
#include "Noncopyable.hpp"

#include <memory>
#include <algorithm>
#include <functional>
#include <memory>
#include <utility>
#include <vector>

// singal-slot, observer-subsriber, event-delegate whatever pattern
//
// usage:
//      Event<T...> is an event producer, produces an event when called with Notify or operator()
//
//      EventObserverScopeTyped<T...> is an event observer, connects to one event with Connect/Disconnect.
//      its type signature must match the connected Events type signature.
//      its callback function can be set with SetCallback or in the ctor.
//      it is disconnected from the Event when destructed.
//      destructing an Event disconnects the observer.
//
//      SingleEventObserver and EventObserver are untyped versions of EventObserverScopeTyped,
//      but they cannot be Connected/Disconnected from an Event arbitrarily.
//      they are connected with a callback in Observe, disconnected with the destructor.
//      SingleEventObserver can only subsribe to only one Event at a time.
//      EventObserver can subscribe to multiple events. one EventObserver can only observe an Event once.

namespace GVK {

template<typename... ARGS>
class Event;


// for EventObserver
class IEventObserverScope {
public:
    virtual ~IEventObserverScope () = default;
};


template<typename... ARGS>
class EventObserverScopeTyped : public Noncopyable, public IEventObserverScope {
public:
    using Func     = std::function<void (ARGS...)>;
    using Callback = std::shared_ptr<Func>;

    friend class Event<ARGS...>;

private:
    Event<ARGS...>* connectedEvent;
    Callback        callback;

public:
    EventObserverScopeTyped ()
        : connectedEvent (nullptr)
        , callback (nullptr)
    {
    }

    EventObserverScopeTyped (const Func& callback)
        : connectedEvent (nullptr)
        , callback (std::make_shared<Func> (callback))
    {
    }

    virtual ~EventObserverScopeTyped () override
    {
        Disconnect ();
    }

    void SetCallback (const Func& newCallback)
    {
        callback = std::make_shared<Func> (newCallback);
    }

    void Connect (Event<ARGS...>& ev);

    void Disconnect ();

private:
    void Notify (ARGS... args)
    {
        if (callback != nullptr) {
            (*callback) (std::forward<ARGS> (args)...);
        }
    }
};

class IEvent {
public:
    virtual ~IEvent () = default;
};

template<typename... ARGS>
class Event : public Noncopyable, public IEvent {
public:
    friend class EventObserverScopeTyped<ARGS...>;

private:
    std::vector<EventObserverScopeTyped<ARGS...>*> evObservers;

public:
    Event () = default;

    virtual ~Event ()
    {
        for (const auto& ob : evObservers) {
            ob->Disconnect ();
        }
    }

    // for observer

    void Connect (EventObserverScopeTyped<ARGS...>& obs, const std::function<void (ARGS...)>& callback)
    {
        obs.SetCallback (callback);
        obs.Connect (*this);
    }

    // for observervable

    void Notify (ARGS... args) const
    {
        for (auto& obv : evObservers) {
            GVK_ASSERT (obv != nullptr);
            obv->Notify (std::forward<ARGS> (args)...);
        }
    }

    void operator() (ARGS... args) const
    {
        Notify (std::forward<ARGS> (args)...);
    }
};


template<typename... ARGS>
void EventObserverScopeTyped<ARGS...>::Connect (Event<ARGS...>& ev)
{
    if (GVK_ERROR (connectedEvent != nullptr)) {
        // must disconnect first
        return;
    }

    ev.evObservers.push_back (this);
    connectedEvent = &ev;
}


template<typename... ARGS>
void EventObserverScopeTyped<ARGS...>::Disconnect ()
{
    if (connectedEvent != nullptr) {
        connectedEvent->evObservers.erase (std::remove (connectedEvent->evObservers.begin (), connectedEvent->evObservers.end (), this), connectedEvent->evObservers.end ());
        connectedEvent = nullptr;
    }
}


class EventObserver : public Noncopyable {
private:
    std::vector<std::unique_ptr<IEventObserverScope>> observers;
    std::vector<IEvent*>                              observedEvents;

public:
    EventObserver () = default;

    EventObserver (EventObserver&& other) noexcept
        : observers (std::move (other.observers))
        , observedEvents (std::move (other.observedEvents))
    {
    }

    virtual ~EventObserver () = default;

    template<typename... ARGS, typename Observer>
    void Observe (Event<ARGS...>& ev, Observer observer)
    {
        auto it = std::find (observedEvents.begin (), observedEvents.end (), &ev);
        if (GVK_ERROR (it != observedEvents.end ())) {
            // event already observed
            return;
        }

        std::unique_ptr<EventObserverScopeTyped<ARGS...>> obs = std::make_unique<EventObserverScopeTyped<ARGS...>> (observer);
        obs->Connect (ev);
        observers.push_back (std::move (obs));
        observedEvents.push_back (&ev);
    }

    void Disconnect (IEvent& ev)
    {
        auto it = std::find (observedEvents.begin (), observedEvents.end (), &ev);
        if (GVK_ERROR (it == observedEvents.end ())) {
            // event not observed
            return;
        }

        const size_t obsIndex = std::distance (observedEvents.begin (), it);

        observers.erase (observers.begin (), observers.begin () + obsIndex);
        observedEvents.erase (observedEvents.begin (), observedEvents.begin () + obsIndex);
    }
};


class SingleEventObserver {
private:
    std::unique_ptr<IEventObserverScope> observer;
    IEvent*                              observedEvent;

public:
    SingleEventObserver ()
        : observedEvent (nullptr)
    {
    }

    virtual ~SingleEventObserver () = default;

    template<typename... ARGS, typename Observer>
    void Observe (Event<ARGS...>& ev, Observer observerCallback)
    {
        if (GVK_ERROR (observedEvent != nullptr)) {
            return;
        }

        std::unique_ptr<EventObserverScopeTyped<ARGS...>> obs = std::make_unique<EventObserverScopeTyped<ARGS...>> (observerCallback);
        obs->Connect (ev);
        observer      = std::move (obs);
        observedEvent = &ev;
    }

    void Disconnect ()
    {
        if (GVK_ERROR (observedEvent == nullptr)) {
            return;
        }

        observer.reset ();
        observedEvent = nullptr;
    }
};

} // namespace GVK

#endif