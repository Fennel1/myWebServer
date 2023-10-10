#ifndef WEBSERVER_H
#define WEBSERVER_H

#include <arpa/inet.h>

#include "../threadpool/threadpool.h"
#include "../http/http_conn.h"

const int MAX_FD = 65536;           //最大文件描述符
const int MAX_EVENT_NUMBER = 10000; //最大事件数
const int TIMESLOT = 5;             //最小超时单位

class WebServer{
public:
    WebServer();
    ~WebServer();

    void init(int port , std::string user, std::string passWord, std::string databaseName,
              int log_write , int linger, int et, int sql_num,
              int thread_num, int close_log, int actor_model);
    void thread_pool();
    void sql_pool();
    void log_write();
    void et();
    void eventListen();
    void eventLoop();
    void timer(int connfd, struct sockaddr_in client_address);
    void adjust_timer(util_timer *timer);
    void del_timer(util_timer *timer, int socketfd);
    bool dealclientdata();
    bool dealwithsignal(bool &timeout, bool &stop_server);
    void dealwithread(int socketfd);
    void dealwithwrite(int socketfd);

    int port_;
    char *root_;
    int log_write_;
    int close_log_;
    int actorModel_;

    int pipefd_[2];
    int epollfd_;
    http_conn *users_;

    connection_pool *connPool_;
    std::string user_;
    std::string password_;
    std::string databaseName_;
    int sqlNUm_;

    threadpool<http_conn> *pool_;
    int threadNum_;

    epoll_event events_[MAX_EVENT_NUMBER];

    int listenfd_;
    int linger_;
    int et_;
    int listenfdMode_;
    int connfdMode_;

    client_data *users_timer_;
    Utils utils_;
};

#endif