#include "threadpool.h"


template <typename T>
threadpool<T>::threadpool(connection_pool *connPool, int thread_number, int max_requests) : 
connPool_(connPool), thread_number_(thread_number), max_requests_(max_requests), pthreads_(nullptr){
    if (thread_number <= 0 || max_requests <= 0){
        throw std::exception();
    }

    pthreads_ = new pthread_t[thread_number];
    if (pthreads_ == nullptr){
        throw std::exception();
    }

    for (int i=0; i<thread_number; i++){
        if (pthread_create(pthreads_ + i, nullptr, worker, this) != 0){
            delete [] pthreads_;
            throw std::exception();
        }
        if (pthread_detach(pthreads_[i])){
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