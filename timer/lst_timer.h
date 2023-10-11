#ifndef LST_TIMER
#define LST_TIMER

#include <time.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <errno.h>
#include <cstring>
#include <signal.h>
#include <assert.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include "../http/http_conn.h"

class util_timer;

struct client_data{
    sockaddr_in address;
    int sockfd;
    util_timer *timer;
};

class util_timer{
public:
    util_timer() : prev(nullptr), next(nullptr) {}

    time_t expire;
    void (* cb_func)(client_data *);
    client_data *user_data;
    util_timer *prev;
    util_timer *next;
};

class sort_timer_lst{
public:
    sort_timer_lst();
    ~sort_timer_lst();

    void add_timer(util_timer *timer);
    void adjust_timer(util_timer *timer);
    void del_timer(util_timer *timer);
    void tick();

private:
    void add_timer_aux(util_timer *timer, util_timer *lst_head);

    util_timer *head;
    util_timer *tail;
};

class Utils{
public:
    Utils() {};
    ~Utils() {};

    void init(int timeslot);
    int setnonblocking(int fd);
    void addfd(int epollfd, int fd, bool one_shot, bool et);
    static void sig_handler(int sig);
    void addsig(int sig, void(sig_handler)(int), bool reset=true);
    void timer_handler();
    void show_error(int connfd, const char *info);
    
    static int *pipefd_;
    static int epollfd_;
    int timeslot_;
    sort_timer_lst timer_lst_;
};

void cb_func(client_data *user_data);

#endif