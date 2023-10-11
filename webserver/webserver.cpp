#include "webserver.h"

WebServer::WebServer(){
    users_ = new http_conn[MAX_FD];

    char server_path[200];
    getcwd(server_path, 200);
    char root[6] = "/root";
    root_ = (char *)malloc(strlen(server_path)+strlen(root)+1);
    strcpy(root_, server_path);
    strcat(root_, root);

    users_timer_ = new client_data[MAX_FD];
}

WebServer::~WebServer(){
    close(epollfd_);
    close(listenfd_);
    close(pipefd_[0]);
    close(pipefd_[1]);
    delete [] users_;
    delete [] users_timer_;
    delete pool_;
}

void WebServer::init(int port , std::string user, std::string passWord, std::string databaseName,
              int log_write , int linger, int et, int sql_num,
              int thread_num, int close_log, int actor_model){
    port_ = port;
    user_ = user;
    password_ = passWord;
    databaseName_ = databaseName;
    sqlNUm_ = sql_num;
    threadNum_ = thread_num;
    log_write_ = log_write;
    linger_ = linger;
    et_ = et;
    close_log_ = close_log;
    actorModel_ = actor_model;
}

void WebServer::et(){
    if (et_ == 0){
        listenfdMode_ = 0;
        connfdMode_ = 0;
    }
    else if (et_ == 1){
        listenfdMode_ = 0;
        connfdMode_ = 1;
    }
    else if (et_ == 2){
        listenfdMode_ = 1;
        connfdMode_ = 0;
    }
    else if (et_ == 3){
        listenfdMode_ = 1;
        connfdMode_ = 1;
    }
}

void WebServer::log_write(){
    if (close_log_ == 0){
        if (log_write_ == 1){
            log::get_instance()->init("./ServerLog", close_log_, 2000, 800000, 800);
        }
        else{
            log::get_instance()->init("./ServerLog", close_log_, 2000, 800000, 0);
        }
    }
}

void WebServer::sql_pool(){
    connPool_ = connection_pool::GetInstance();
    connPool_->init("localhost", user_, password_, databaseName_, 3306, sqlNUm_, close_log_);
    users_->initmysql_result(connPool_);
}

void WebServer::thread_pool(){
    pool_ = new threadpool<http_conn>(connPool_, actorModel_, threadNum_);
}

void WebServer::eventListen(){
    listenfd_ = socket(PF_INET, SOCK_STREAM, 0);
    assert(listenfd_ >= 0);

    if (linger_ == 0){
        struct linger tmp = {0, 1};
        setsockopt(listenfd_, SOL_SOCKET, SO_LINGER, &tmp, sizeof(tmp));
    }
    else if (linger_ == 1){
        struct linger tmp = {1, 1};
        setsockopt(linger_, SOL_SOCKET, SO_LINGER, &tmp, sizeof(tmp));
    }

    struct sockaddr_in address;
    bzero(&address, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_ANY);
    address.sin_port = htons(port_);

    int ret = 0, flag = 1;
    setsockopt(listenfd_, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(flag));
    ret = bind(listenfd_, (struct sockaddr *)&address, sizeof(address));
    assert(ret >= 0);
    ret = listen(listenfd_, 5);
    assert(ret >= 0);

    utils_.init(TIMESLOT);

    epoll_event events[MAX_EVENT_NUMBER];
    epollfd_ = epoll_create(5);
    assert(epollfd_ != -1);

    utils_.addfd(epollfd_, listenfd_, false, listenfdMode_);
    http_conn::epollfd_ = epollfd_;

    ret = socketpair(PF_UNIX, SOCK_STREAM, 0, pipefd_);
    assert(ret != -1);
    utils_.setnonblocking(pipefd_[1]);
    utils_.addfd(epollfd_, pipefd_[0], false, 0);

    utils_.addsig(SIGPIPE, SIG_IGN);
    utils_.addsig(SIGALRM, utils_.sig_handler, false);
    utils_.addsig(SIGTERM, utils_.sig_handler, false);

    alarm(TIMESLOT);

    Utils::pipefd_ = pipefd_;
    Utils::epollfd_ = epollfd_;
}

void WebServer::timer(int connfd, struct sockaddr_in client_address){
    users_[connfd].init(connfd, client_address, root_, connfdMode_, close_log_, user_, password_, databaseName_);

    //创建定时器，设置回调函数和超时时间，绑定用户数据，将定时器添加到链表中
    users_timer_[connfd].address = client_address;
    users_timer_[connfd].sockfd = connfd;
    util_timer *timer = new util_timer;
    timer->user_data = &users_timer_[connfd];
    timer->cb_func = cb_func;
    time_t cur = time(NULL);
    timer->expire = cur + 3*TIMESLOT;
    users_timer_[connfd].timer = timer;
    utils_.timer_lst_.add_timer(timer);
}

void WebServer::adjust_timer(util_timer *timer){
    //若有数据传输，则将定时器往后延迟3个单位
    //并对新的定时器在链表上的位置进行调整
    time_t cur = time(NULL);
    timer->expire = cur + 3*TIMESLOT;
    utils_.timer_lst_.adjust_timer(timer);

    // LOG_INFO("%s", "adjust timer once");
}

void WebServer::del_timer(util_timer *timer, int socketfd){
    timer->cb_func(&users_timer_[socketfd]);
    if (timer){
        utils_.timer_lst_.del_timer(timer);
    }

    // LOG_INFO("close fd %d", users_timer[sockfd].sockfd);
}

bool WebServer::dealclientdata(){
    struct sockaddr_in client_address;
    socklen_t client_addrlength = sizeof(client_address);
    if (listenfdMode_ == 0){
        int connfd = accept(listenfd_, (struct sockaddr *)&client_address, &client_addrlength);
        if (connfd < 0){
            LOG_ERROR("%s:errno is:%d", "accept error", errno);
            return false;
        }
        if (http_conn::user_count_ >= MAX_FD){
            utils_.show_error(connfd, "Internal server busy");
            LOG_ERROR("%s", "Internal server busy");
            return false;
        }
        timer(connfd, client_address);
    }
    else{
        while(true){
            int connfd = accept(listenfd_, (struct sockaddr *)&client_address, &client_addrlength);
            if (connfd < 0){
                LOG_ERROR("%s:errno is:%d", "accept error", errno);
                return false;
            }
            if (http_conn::user_count_ >= MAX_FD){
                utils_.show_error(connfd, "Internal server busy");
                LOG_ERROR("%s", "Internal server busy");
                return false;
            }
            timer(connfd, client_address);
        }
        return false;
    }
    return true;
}

bool WebServer::dealwithsignal(bool &timeout, bool &stop_server){
    int ret = 0;
    char signals[1024];
    ret = recv(pipefd_[0], signals, sizeof(signals), 0);
    if (ret == -1 || ret == 0){
        return false;
    }
    for (int i=0; i<ret; i++){
        switch (signals[i]){
            case SIGALRM:
                timeout = true;
                break;
            case SIGTERM:
                stop_server = true;
                break;
        }
    }
    return true;
}

void WebServer::dealwithread(int socketfd){
    util_timer *timer = users_timer_[socketfd].timer;

    if (actorModel_ == 1){   // reactor
        if (timer){
            adjust_timer(timer);
        }
        pool_->append(users_+socketfd, 0);
        while(true){
            if (users_[socketfd].improv_ == 1){
                if (users_[socketfd].timer_flag_ == 1){
                    del_timer(timer, socketfd);
                    users_[socketfd].timer_flag_ = 0;
                }
                users_[socketfd].improv_ = 0;
                break;
            }
        }
    }
    else{   // proactor
        if (users_[socketfd].read_once()){
            LOG_INFO("deal with the client(%s)", inet_ntoa(users_[socketfd].get_address()->sin_addr));
            pool_->append(users_+socketfd, 0);
            if (timer){
                adjust_timer(timer);
            }
        }
        else{
            del_timer(timer, socketfd);
        }
    }
}

void WebServer::dealwithwrite(int socketfd){
    util_timer *timer = users_timer_[socketfd].timer;

    if (actorModel_ == 1){
        // reactor
        if (timer){
            adjust_timer(timer);
        }
        pool_->append(users_+socketfd, 1);
        while(true){
            if (users_[socketfd].improv_ == 1){
                if (users_[socketfd].timer_flag_ == 1){
                    del_timer(timer, socketfd);
                    users_[socketfd].timer_flag_ = 0;
                }
                users_[socketfd].improv_ = 0;
                break;
            }
        }
    }
    else{
        // proactor
        if (users_[socketfd].write()){
            LOG_INFO("send data to the client(%s)", inet_ntoa(users_[socketfd].get_address()->sin_addr));
            if (timer){
                adjust_timer(timer);
            }
        }
        else{
            del_timer(timer, socketfd);
        }
    }
}

void WebServer::eventLoop(){
    bool timeout = false;
    bool stop_server = false;

    while (!stop_server){
        int number = epoll_wait(epollfd_, events_, MAX_EVENT_NUMBER, -1);
        if (number < 0 && errno != EINTR){
            LOG_ERROR("%s", "epoll failure");
            break;
        }

        for (int i=0; i<number; i++){
            int socketfd = events_[i].data.fd;

            if (socketfd == listenfd_){ //处理新到的客户连接
                bool flag = dealclientdata();
                if (!flag){
                    continue;
                }
            }
            else if (events_[i].events & (EPOLLRDHUP | EPOLLHUP | EPOLLERR)){   //服务器端关闭连接，移除对应的定时器
                util_timer *timer = users_timer_[socketfd].timer;
                del_timer(timer, socketfd);
            }
            else if (socketfd == pipefd_[0] && events_[i].events & EPOLLIN){    //处理信号
                bool flag = dealwithsignal(timeout, stop_server);
                if (!flag){
                    LOG_ERROR("%s", "dealclientdata failure");
                }
            }
            else if (events_[i].events & EPOLLIN){
                dealwithread(socketfd);
            }
            else if (events_[i].events & EPOLLOUT){
                dealwithwrite(socketfd);
            }
        }

        if (timeout){
            utils_.timer_handler();
            LOG_INFO("%s", "timer tick");
            timeout = false;
        }
    }
}