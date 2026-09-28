// EPOS Synchronizer Components

#ifndef __synchronizer_h
#define __synchronizer_h

#include <architecture.h>
#include <utility/handler.h>
#include <process.h>

__BEGIN_SYS

class Synchronizer_Common
{
protected:
    static const bool multitask = Traits<System>::multitask;

    typedef Thread::Queue Queue;

protected:
    Synchronizer_Common() {}
    ~Synchronizer_Common() {
        Thread::lock();
        while(!_granted.empty()) {
            Queue::Element * e = _granted.remove();
            if(e)
                delete e;
        }
        if(!_waiting.empty())
            db<Synchronizer>(WRN) << "~Synchronizer(this=" << this << ") called with active blocked clients!" << endl;
        wakeup_all();
        Thread::unlock();
    }

    // Atomic operations
    bool tsl(volatile bool & lock) { return CPU::tsl(lock); }
    long finc(volatile long & number) { return CPU::finc(number); }
    long fdec(volatile long & number) { return CPU::fdec(number); }

    // Thread operations
    void lock_for_acquiring() { Thread::lock(); /*Thread::prioritize(&_granted);*/ }
    void unlock_for_acquiring() { /*_granted.insert(new (SYSTEM) Queue::Element(Thread::running())); */Thread::unlock(); }
    void lock_for_releasing() { Thread::lock(); /*Queue::Element * e = _granted.remove(); if(e) delete e; Thread::deprioritize(&_granted); Thread::deprioritize(&_waiting);*/ }
    void unlock_for_releasing() { Thread::unlock(); }

    void sleep() { Thread::sleep(&_waiting); }
    void wakeup() { Thread::wakeup(&_waiting); }
    void wakeup_all() { Thread::wakeup_all(&_waiting); }

protected:
    Queue _waiting;
    Queue _granted;
};


class Mutex: protected Synchronizer_Common
{
public:
    Mutex();
    ~Mutex();

    void lock();
    void unlock();

private:
    alignas (int) volatile bool _locked;
};


class Semaphore: protected Synchronizer_Common
{
public:
    Semaphore(long v = 1);
    ~Semaphore();

    void p();
    void v();

private:
    volatile long _value;
};


// This is actually no Condition Variable
// check http://www.cs.duke.edu/courses/spring01/cps110/slides/sem/sld002.htm
class Condition: protected Synchronizer_Common
{
public:
    Condition();
    ~Condition();

    void wait();
    void signal();
    void broadcast();
};


// An event handler that triggers a mutex (see handler.h)
class Mutex_Handler: public Handler
{
public:
    Mutex_Handler(Mutex * h) : _handler(h) {}
    ~Mutex_Handler() {}

    void operator()() { _handler->unlock(); }

private:
    Mutex * _handler;
};

// An event handler that triggers a semaphore (see handler.h)
class Semaphore_Handler: public Handler
{
public:
    Semaphore_Handler(Semaphore * h) : _handler(h) {}
    ~Semaphore_Handler() {}

    void operator()() { _handler->v(); }

private:
    Semaphore * _handler;
};

// An event handler that triggers a condition variable (see handler.h)
class Condition_Handler: public Handler
{
public:
    Condition_Handler(Condition * h) : _handler(h) {}
    ~Condition_Handler() {}

    void operator()() { _handler->signal(); }

private:
    Condition * _handler;
};


// Conditional Observer x Conditionally Observed with Data decoupled by a Semaphore
template<typename D, typename C = void>
class Concurrent_Observer;

template<typename D, typename C = void>
class Concurrent_Observed
{
    friend class Concurrent_Observer<D, C>;

private:
    typedef typename Simple_Ordered_List<Concurrent_Observer<D, C>, C>::Element Element;

public:
    typedef D Observed_Data;
    typedef C Observing_Condition;

public:
    Concurrent_Observed() {
        db<Observers, Semaphore>(TRC) << "Concurrent_Observed() => " << this << endl;
    }

    ~Concurrent_Observed() {
        db<Observers, Semaphore>(TRC) << "~Concurrent_Observed(this=" << this << ")" << endl;
    }

    void attach(Concurrent_Observer<D, C> * o, const C & c) {
        db<Observers, Semaphore>(TRC) << "Concurrent_Observed::attach(obs=" << o << ",cond=" << c << ")" << endl;

        o->_link = Element(o, c);
        _observers.insert(&o->_link);
    }

    void detach(Concurrent_Observer<D, C> * o, const C & c) {
        db<Observers, Semaphore>(TRC) << "Concurrent_Observed::detach(obs=" << o << ",cond=" << c << ")" << endl;

        _observers.remove(&o->_link);
    }

    bool notify(const C & c, D * d) {
        bool notified = false;

        db<Observers, Semaphore>(TRC) << "Concurrent_Observed::notify(this=" << this << ",cond=" << c << ")" << endl;

        for(Element * e = _observers.head(); e; e = e->next()) {
            if(e->rank() == c) {
                db<Observers, Semaphore>(INF) << "Observed::notify(this=" << this << ",obs=" << e->object() << ")" << endl;
                e->object()->update(c, d);
                notified = true;
            }
        }

        return notified;
    }

private:
    Simple_Ordered_List<Concurrent_Observer<D, C>, C> _observers;
};

template<typename D, typename C>
class Concurrent_Observer
{
    friend class Concurrent_Observed<D, C>;

public:
    typedef D Observed_Data;
    typedef C Observing_Condition;

public:
    Concurrent_Observer(): _semaphore(0), _link(this) {
        db<Observers>(TRC) << "Observer() => " << this << endl;
    }

    ~Concurrent_Observer() {
        db<Observers>(TRC) << "~Observer(this=" << this << ")" << endl;
    }

    void update(const C & c, D * d) {
        _list.insert(d->lext());
        _semaphore.v();
    }

    D * updated() {
        _semaphore.p();
        return _list.remove()->object();
    }

private:
    Semaphore _semaphore;
    Simple_List<D> _list;
    typename Concurrent_Observed<D, C>::Element _link;
};

__END_SYS

#endif
