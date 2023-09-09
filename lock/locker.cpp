#include "locker.h"

sem::sem(){
    if (sem_init(&m_sem_, 0, 0) != 0){
        throw std::exception();
    }
}

sem::sem(int num){
    if (sem_init(&m_sem_, 0, num) != 0){
        throw std::exception();
    }
}

sem::~sem(){
    sem_destroy(&m_sem_);
}

bool sem::wait(){
    return sem_wait(&m_sem_) == 0;
}

bool sem::post(){
    return sem_post(&m_sem_) == 0;
}



locker::locker(){
    if (pthread_mutex_init(&m_mutex_, NULL) != 0){
        throw std::exception();
    }
}

locker::~locker(){
    pthread_mutex_destroy(&m_mutex_);
}

bool locker::lock(){
    return pthread_mutex_lock(&m_mutex_) == 0;
}

bool locker::unlock(){
    return pthread_mutex_unlock(&m_mutex_) == 0;
}

pthread_mutex_t *locker::get(){
    return &m_mutex_;
}



cond::cond(){
    if (pthread_cond_init(&m_cond_, NULL) != 0){
        throw std::exception();
    }
}

cond::~cond(){
    pthread_cond_destroy(&m_cond_);
}

bool cond::wait(pthread_mutex_t *m_mutex){
    int ret = 0;
    ret = pthread_cond_wait(&m_cond_, m_mutex);
    return ret == 0;
}

bool cond::timewait(pthread_mutex_t *m_mutex, struct timespec t){
    int ret = 0;
    ret = pthread_cond_timedwait(&m_cond_, m_mutex, &t);
    return ret == 0;
}

bool cond::signal(){
    return pthread_cond_signal(&m_cond_) == 0;
}

bool cond::broadcast(){
    return pthread_cond_broadcast(&m_cond_) == 0;
}