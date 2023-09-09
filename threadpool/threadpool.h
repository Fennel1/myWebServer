#ifndef THREADPOOL_H
#define THREADPOOL_H

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
    pthread *pthreads_;
    std::list<T *> workqueue_;
    locker queuelocker_;
    sem queuestat_;
    connection_pool *connPool_;
};

#endif