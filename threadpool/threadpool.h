#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <iostream>
#include <list>
#include <exception>
#include <pthread.h>
#include "../mysql/mysql_connection_pool.h"
#include "../lock/locker.h"

template <typename T>
class threadpool{
public:
    threadpool(connection_pool *connPool, int thread_number = 8, int max_requests = 10000);
    ~threadpool();
    bool append(T *request);

private:
    static void *worker(void *arg);
    void run();

    int thread_number_;
    int max_requests_;
    pthread_t *pthreads_;
    std::list<T *> workqueue_;
    locker queuelocker_;
    sem queuestat_;
    connection_pool *connPool_;
};

template <typename T>
threadpool<T>::threadpool(connection_pool *connPool, int thread_number, int max_requests) : 
connPool_(connPool), thread_number_(thread_number), max_requests_(max_requests), pthreads_(nullptr){
    if (thread_number <= 0 || max_requests <= 0){
        std::cout << "thread_number or max_requests Error" << std::endl;
        throw std::exception();
    }

    pthreads_ = new pthread_t[thread_number];
    if (pthreads_ == nullptr){
        std::cout << "new pthreads_ Error" << std::endl;
        throw std::exception();
    }

    for (int i=0; i<thread_number; i++){
        if (pthread_create(pthreads_ + i, nullptr, worker, this) != 0){
            std::cout << "pthread_create Error" << std::endl;
            delete [] pthreads_;
            throw std::exception();
        }
        if (pthread_detach(pthreads_[i])){
            std::cout << "pthread_detach Error" << std::endl;
            delete [] pthreads_;
            throw std::exception();
        }
    }
}

template <typename T>
threadpool<T>::~threadpool(){
    delete [] pthreads_;
}

template <typename T>
bool threadpool<T>::append(T *request){
    queuelocker_.lock();

    if (workqueue_.size() >= max_requests_){
        queuelocker_.unlock();
        return false;
    }

    workqueue_.push_back(request);
    queuelocker_.unlock();
    queuestat_.post();
    return true;
}

template <typename T>
void *threadpool<T>::worker(void *arg){
    threadpool *pool = (threadpool *)arg;
    pool->run();
    return pool;
}

template <typename T>
void threadpool<T>::run(){
    while(true){
        queuestat_.wait();
        queuelocker_.lock();

        if (workqueue_.empty()){
            queuelocker_.unlock();
            continue;
        }
        T *request = workqueue_.front();
        workqueue_.pop_front();

        queuelocker_.unlock();
        if (request == nullptr){
            continue;
        }

        connectionRAII mysqlcon(&request->mysql_, connPool_);
        request->process();
    }
}

#endif