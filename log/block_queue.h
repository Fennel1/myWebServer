#ifndef BLOCK_QUEUE_H
#define BLOCK_QUEUE_H

#include <cstdlib>
#include <sys/time.h>

#include "../lock/locker.h"

template <class T>
class block_queue{
public:
    block_queue(int max_size=1000);
    ~block_queue();

    void clear();
    bool full();
    bool empty();
    bool front(T &val);
    bool back(T &val);
    int size();
    int max_size();
    bool push(const T &item);
    bool pop(T &item);
    bool pop(T &item, int ms_timeout);

private:
    locker mutex_;
    cond cond_;
    T *array_;
    int size_;
    int max_size_;
    int front_;
    int back_;
};

#endif
