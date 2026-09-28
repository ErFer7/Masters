// EPOS Synchronizer Component Test Program

#include <machine.h>
#include <time.h>
#include <synchronizer.h>
#include <process.h>
#include <memory.h>

using namespace EPOS;

const int iterations = 128;

OStream cout;

int consumer_main(int argc, char ** arvg);
int producer();
int consumer();
void produce(unsigned long long n);
void consume(unsigned long long n);
unsigned long long busy_wait(unsigned long long n);

const int BUF_SIZE = 16;

char * buffer;
Semaphore * empty;
Semaphore * full;

int main(int argc, char ** arvg)
{
    cout << "Producer x Consumer\n" << endl;

    cout << "I'm the first Task." << endl;

#ifndef __kernel__

    buffer = new char[BUF_SIZE];
    empty = new Semaphore(BUF_SIZE);
    full = new Semaphore(0);

    Thread * cons = new Thread(&consumer);

#else

    Segment * seg = new Segment(BUF_SIZE + 2 * sizeof(Id));
    if(seg)
        cout << "Shared buffer segment created." << endl;
    else {
        cout << "Can't allocated the shared buffer!" << endl;
        return -1;
    }

    buffer = Task::self()->address_space()->attach(seg);
    if(buffer)
        cout << "Shared buffer segment attached." << endl;
    else {
        cout << "Can't attached the shared buffer!" << endl;
        return -1;
    }

    empty = new Semaphore(BUF_SIZE);
    full = new Semaphore(0);

    *reinterpret_cast<Id *>(buffer) = empty->id();
    *reinterpret_cast<Id *>(buffer + sizeof(Id)) = full->id();
    buffer += 2 * sizeof(Id);

    Task * cons = new Task(Task::self(), &consumer_main, seg->id().unit(), 0);
    if(cons)
        cout << "Consumer task created." << endl;
    else {
        cout << "Can't create the consumer!" << endl;
        return -1;
    }

#endif

    Thread * prod = new Thread(&producer);

    cons->join();
    prod->join();

    cout << "The end!" << endl;

    delete cons;
    delete prod;

    return 0;
}

#ifdef __kernel__

int consumer_main(int argc, char ** arvg)
{
    OStream cout;

    cout << "\nI'm the second Task." << endl;

    Id id(EPOS::S::Type<EPOS::S::Segment>::ID, argc);
    Segment * seg = new Segment(id);
    if(seg)
        cout << "Got the shared buffer segment created by the Producer." << endl;
    else {
        cout << "Can't get the shared buffer!" << endl;
        return -1;
    }

    buffer = Task::self()->address_space()->attach(seg);
    if(buffer)
        cout << "Attached the shared buffer segment created by the Producer. \n" << endl;
    else {
        cout << "Can't attached the shared buffer!" << endl;
        return -1;
    }

    Id e = *reinterpret_cast<Id *>(buffer);
    Id f = *reinterpret_cast<Id *>(buffer + sizeof(Id));
    buffer += 2 * sizeof(Id);

    empty = new Semaphore(e);
    full = new Semaphore(f);

    return consumer();
}

#endif

int producer()
{
    cout << "I'm the Producer." << endl;

    int in = 0;

    for(int i = 0; i < iterations; i++) {
        empty->p();
        produce(100000);
        buffer[in] = 'a' + in;
        cout << "P->" << buffer[in] << " ";
        in = (in + 1) % BUF_SIZE;
        full->v();
    }

    return 0;
}


int consumer()
{
    cout << "I'm the Consumer." << endl;

    int out = 0;

    for(int i = 0; i < iterations; i++) {
        full->p();
        cout << "C<-" << buffer[out] << " ";
        out = (out + 1) % BUF_SIZE;
        produce(100000);
        empty->v();
    }

    return 0;
}


void produce(unsigned long long n) {
    busy_wait(n);
}


void consume(unsigned long long n) {
    busy_wait(n);
}


unsigned long long busy_wait(unsigned long long n)
{
    volatile unsigned long long v;
    for(unsigned long long int j = 0; j < 20 * n; j++)
        v &= 2 ^ j;
    return v;
}
