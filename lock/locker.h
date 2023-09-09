#ifndef LOCKER_H
#define LOCKER_H

#include <pthread.h>
#include <semaphore.h>
#include <exception>

class sem{
public:
    sem();
    sem(int num);
    ~sem();

    bool wait();
    bool post();

private:
    sem_t m_sem_;
};

class locker{
public:
    locker();
    ~locker();

    bool lock();
    bool unlock();
    pthread_mutex_t *get();

private:
    pthread_mutex_t m_mutex_;
};

class cond{
public:
    cond();
    ~cond();

    bool wait(pthread_mutex_t *m_mutex);
    bool timewait(pthread_mutex_t *m_mutex, struct timespec t);

    bool signal();
    bool broadcast();

private:
    pthread_cond_t m_cond_;
};

#endif