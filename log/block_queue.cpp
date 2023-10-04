#include "block_queue.h"

template <class T>
block_queue<T>::block_queue(int max_size){
    if (max_size <= 0){
        exit(-1);
    }

    max_size_ = max_size;
    array_ = new T[max_size_];
    size_ = 0;
    front_ = back_ = -1;
}

template <class T>
void block_queue<T>::clear(){
    mutex_.lock();

    size_ = 0;
    front_ = back_ = -1;

    mutex_.unlock();
}

template <class T>
block_queue<T>::~block_queue(){
    mutex_.lock();

    if (array_){
        delete [] array_;
    }

    mutex_.unlock();
}

template <class T>
bool block_queue<T>::full(){
    mutex_.lock();

    if (size_ >= max_size_){
        mutex_.unlock();
        return true;
    }

    mutex_.unlock();
    return false;
}

template <class T>
bool block_queue<T>::empty(){
    mutex_.lock();

    if (size_ == 0){
        mutex_.unlock();
        return true;
    }

    mutex_.unlock();
    return false;
}

template <class T>
bool block_queue<T>::front(T &val){
    mutex_.lock();

    if (size_ == 0){
        mutex_.unlock();
        return false;
    }

    val = array_[front_];
    mutex_.unlock();
    return true;
}

template <class T>
bool block_queue<T>::back(T &val){
    mutex_.lock();

    if (size_ == 0){
        mutex_.unlock();
        return false;
    }

    val = array_[back_];
    mutex_.unlock();
    return true;
}

template <class T>
int block_queue<T>::size(){
    int tmp;
    mutex_.lock();
    tmp = size_;
    mutex_.unlock();
    return tmp;
}

template <class T>
int block_queue<T>::max_size(){
    int tmp;
    mutex_.lock();
    tmp = max_size_;
    mutex_.unlock();
    return tmp;
}

template <class T>
bool block_queue<T>::push(const T &item){
    mutex_.lock();
    
    if (size_ >= max_size_){
        cond_.broadcast();
        mutex_.unlock();
        return false;
    }

    back_ = (back_+1) % max_size_;
    array_[back] = item;
    size_++;

    cond_.broadcast();
    mutex_.unlock();
    return true;
}

template <class T>
bool block_queue<T>::pop(T &item){
    mutex_.lock();
    
    if (size_ <= 0){
        if (!cond_.wait(mutex_.get())){
            mutex_.unlock();
            return false;
        }
    }

    front_ = (front_+1) % max_size_;
    item = array_[front_];
    size_--;

    mutex_.unlock();
    return true;
}

template <class T>
bool block_queue<T>::pop(T &item, int ms_timeout){
    struct timespec t = {0, 0};
    struct timeval now = {0, 0};
    gettimeofday(&now, NULL);

    mutex_.lock();

    if (size_ <= 0){
        t.tv_nsec = now.tv_sec + ms_timeout / 1000;
        t.tv_nsec = (ms_timeout % 1000) * 1000;
        if (!cond_.timewait(mutex_.get(), t)){
            mutex_.unlock();
            return false;
        }
    }
    if (size_ <= 0){
        mutex_.unlock();
        return false;
    }

    front_ = (front_+1) % max_size_;
    item = array_[front_];
    size_--;

    mutex_.unlock();
    return true;
}