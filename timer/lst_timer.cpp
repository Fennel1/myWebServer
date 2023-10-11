#include "lst_timer.h"

sort_timer_lst::sort_timer_lst(){
    head = nullptr;
    tail = nullptr;
}

sort_timer_lst::~sort_timer_lst(){
    util_timer *tmp = head;
    while(tmp){
        head = tmp->next;
        delete tmp;
        tmp = head;
    }
}

void sort_timer_lst::add_timer(util_timer *timer){
    if (!timer){
        return;
    }
    if (!head){
        head = tail = timer;
        return;
    }
    if (timer->expire < head->expire){
        timer->next = head;
        head->prev = timer;
        head = timer;
        return;
    }
    add_timer_aux(timer, head);
}

void sort_timer_lst::add_timer_aux(util_timer *timer, util_timer *lst_head){
    util_timer *prev = lst_head;
    util_timer *cur = prev->next;
    while(cur){
        if (timer->expire < cur->expire){
            prev->next = timer;
            timer->next = cur;
            cur->prev = timer;
            timer->prev = prev;
            break;
        }
        prev = cur;
        cur = cur->next;
    }
    if (!cur){
        prev->next = timer;
        timer->prev = prev;
        timer->next = nullptr;
        tail = timer;
    }
}

void sort_timer_lst::adjust_timer(util_timer *timer){
    if (!timer){
        return;
    }
    util_timer *cur = timer->next;
    if (!cur || (timer->expire < cur->expire)){
        return;
    }
    if (timer == head){
        head = head->next;
        head->prev = nullptr;
        timer->next = nullptr;
        add_timer_aux(timer, head);
    }
    else{
        timer->prev->next = timer->next;
        timer->next->prev = timer->prev;
        add_timer_aux(timer, timer->next);
    }
}

void sort_timer_lst::del_timer(util_timer *timer){
    if (!timer){
        return;
    }
    if (head == tail && head == timer){
        delete timer;
        head = tail = nullptr;
        return;
    }
    if (head == timer){
        head = head->next;
        head->prev = nullptr;
        delete timer;
        return;
    }
    if (tail == timer){
        tail = tail->prev;
        tail->next = nullptr;
        delete timer;
        return;
    }
    timer->prev->next = timer->next;
    timer->next->prev = timer->prev;
    delete timer;
}

void sort_timer_lst::tick(){
    if (!head){
        return;
    }
    time_t cur_time = time(NULL);
    util_timer *tmp = head;
    while(tmp){
        if (cur_time < tmp->expire){
            break;
        }
        tmp->cb_func(tmp->user_data);
        head = tmp->next;
        if (head){
            head->prev = nullptr;
        }
        delete tmp;
        tmp = head;
    }
}

void Utils::init(int timeslot){
    timeslot_ = timeslot;
}

int Utils::setnonblocking(int fd){
    int old_option = fcntl(fd, F_GETFL);
    int new_option = old_option | O_NONBLOCK;
    fcntl(fd, F_SETFL, new_option);
    return old_option;
}

void Utils::addfd(int epollfd, int fd, bool one_shot, bool et){
    epoll_event event;
    event.data.fd = fd;

    if (et){
        event.events = EPOLLIN | EPOLLET | EPOLLRDHUP;
    }
    else{
        event.events = EPOLLIN | EPOLLRDHUP;
    }
    if (one_shot){
        event.events |= EPOLLONESHOT;
    }
        
    epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &event);
    setnonblocking(fd);
}

void Utils::sig_handler(int sig){
    int save_errno = errno;
    int msg = sig;
    send(pipefd_[1], (char *)&msg, 1, 0);
    errno = save_errno;
}

void Utils::addsig(int sig, void(handler)(int), bool reset)
{
    struct sigaction sa;
    memset(&sa, '\0', sizeof(sa));
    sa.sa_handler = handler;
    if (reset){
        sa.sa_flags |= SA_RESTART;
    }
    sigfillset(&sa.sa_mask);
    assert(sigaction(sig, &sa, NULL) != -1);
}

void Utils::timer_handler()
{
    timer_lst_.tick();
    alarm(timeslot_);
}

void Utils::show_error(int connfd, const char *info)
{
    send(connfd, info, strlen(info), 0);
    close(connfd);
}

int *Utils::pipefd_ = 0;
int Utils::epollfd_ = 0;

class Utils;
void cb_func(client_data *user_data)
{
    epoll_ctl(Utils::epollfd_, EPOLL_CTL_DEL, user_data->sockfd, 0);
    assert(user_data);
    close(user_data->sockfd);
    http_conn::user_count_--;
}