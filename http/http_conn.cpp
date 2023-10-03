#include "http_conn.h"

const char *ok_200_title = "OK";
const char *error_400_title = "Bad Request";
const char *error_400_form = "Your request has bad syntax or is inherently impossible to staisfy.\n";
const char *error_403_title = "Forbidden";
const char *error_403_form = "You do not have permission to get file form this server.\n";
const char *error_404_title = "Not Found";
const char *error_404_form = "The requested file was not found on this server.\n";
const char *error_500_title = "Internal Error";
const char *error_500_form = "There was an unusual problem serving the request file.\n";

locker lock;

//对文件描述符设置非阻塞
int setnonblocking(int fd){
    int old_option = fcntl(fd, F_GETFL);
    int new_option = old_option | O_NONBLOCK;
    fcntl(fd, F_SETFL, new_option);
    return old_option;
}

//将内核事件表注册读事件，ET模式，选择开启EPOLLONESHOT
void addfd(int epollfd, int fd, bool one_shot, int et){
    epoll_event event;
    event.data.fd = fd;

    if (et == 1) {
        event.events = EPOLLIN | EPOLLET | EPOLLHUP;
    }
    else {
        event.events = EPOLLIN | EPOLLHUP;
    }

    if (one_shot) {
        event.events |= EPOLLONESHOT;
    }

    epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &event);
    setnonblocking(fd);
}

void removefd(int epollfd, int fd){
    epoll_ctl(epollfd, EPOLL_CTL_DEL, fd, 0);
    close(fd);
}

void modfd(int epollfd, int fd, int ev, int et){
    epoll_event event;
    event.data.fd = fd;

    if (et == 1) {
        event.events = EPOLLIN | EPOLLET | EPOLLHUP;
    }
    else {
        event.events = EPOLLIN | EPOLLHUP;
    }

    epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &event);
}

int http_conn::user_count_ = 0;
int http_conn::epollfd_ = -1;

void http_conn::init(){
    mysql_ = nullptr;
    bytes_to_send_ = 0;
    bytes_have_send_ = 0;
    check_state_ = CHECK_STATE_REQUESTLINE;
    linger_ = false;
    method_ = GET;
    url_ = 0;
    version_ = 0;
    content_len_ = 0;
    host_ = 0;
    start_line_ = 0;
    checked_idx_ = 0;
    read_idx_ = 0;
    write_idx_ = 0;
    cgi_ = 0;
    state_ = 0;
    timer_flag_ = 0;
    improv_ = 0;

    memset(read_buf_, '\0', READ_BUFFER_SIZE);
    memset(write_buf_, '\0', WRITE_BUFFER_SIZE);
    memset(real_file_, '\0', FILENAME_LEN);
}

void http_conn::init(int socketfd, const sockaddr_in &addr, char *root, int et,
                     int close_log, string user, string passwd, string sqlname){
    socketfd_ = socketfd;
    address_ = addr;

    addfd(epollfd_, socketfd_, true, et);
    user_count_++;

    doc_root = root;
    et_ = et;
    close_log_ = close_log;

    strcpy(sql_user_, user.c_str());
    strcpy(sql_passwd_, passwd.c_str());
    strcpy(sql_name_, sqlname.c_str());

    init();
}

void http_conn::close_conn(bool real_close){
    if (real_close && (socketfd_ != -1)){
        printf("close %d\n", socketfd_);
        removefd(epollfd_, socketfd_);
        socketfd_ = -1;
        user_count_--;
    }
}

//从状态机，用于分析出一行内容
http_conn::LINE_STATUS http_conn::parse_line(){
    char tmp;
    for (; checked_idx_<read_idx_; checked_idx_++){
        tmp = read_buf_[checked_idx_];
        if (tmp == '\r'){
            if (checked_idx_+1 == read_idx_){
                return LINE_OPEN;
            }
            else if (read_buf_[checked_idx_+1] == '\n'){
                read_buf_[checked_idx_++] = '\0';
                read_buf_[checked_idx_++] = '\0';
                return LINE_OK;
            }
            else{
                return LINE_BAD;
            }
        }
        else if (tmp == '\n'){
            if (checked_idx_ > 1 && read_buf_[checked_idx_-1] == '\r'){
                read_buf_[checked_idx_-1] = '\0';
                read_buf_[checked_idx_++] = '\0';
                return LINE_OK;
            }
            else{
                return LINE_BAD;
            }
        }
    }
    return LINE_OPEN;
}

//循环读取客户数据，直到无数据可读或对方关闭连接
//非阻塞ET工作模式下，需要一次性将数据读完
bool http_conn::read_once(){
    if (read_idx_ >= READ_BUFFER_SIZE){
        return false;
    }

    int bytes_read = 0;
    if (et == 1){
        while(true){
            bytes_read = recv(socketfd_, read_buf_+read_idx_, READ_BUFFER_SIZE-read_idx_, 0);
            if (bytes_read == -1){
                if (errno == EAGAIN || errno == EWOULDBLOCK){
                    break;
                }
                return false;
            }
            else if (bytes_read == 0){
                return false;
            }
            read_idx_ += bytes_read;
        }
        return true;
    }
    else{
        bytes_read = recv(m_sockfd, m_read_buf + m_read_idx, READ_BUFFER_SIZE - m_read_idx, 0);
        if (bytes_read <= 0){
            return false;
        }
        m_read_idx += bytes_read;
        return true;
    }
}

//解析http请求行，获得请求方法，目标url及http版本号
http_conn::HTTP_CODE http_conn::parse_request_line(char *text){
    url_ = strpbrk(text, " \t");
    if (!url_){
        return BAD_REQUEST;
    }
    *url_++ = '\0';

    char *method = text;
    if (strcasecmp(method, "GET") == 0){
        method_ = GET;
    }
    else if (strcasecmp(method, "POST") == 0){
        method_ = POST;
        cgi_ = 1;
    }
    else{
        return BAD_REQUEST;
    }

    url_ += strspn(url_, " \t");
    version_ = strpbrk(url_, " \t");
    if (!version_){
        return BAD_REQUEST;
    }
    *version_++ = '\0';
    version_ += strspn(version_, " \t");
    if (strcasecmp(version_, "HTTP/1.1") != 0){
        return BAD_REQUEST;
    }

    if (strncasecmp(url_, "http://", 7) == 0){
        url_ += 7;
        url_ = strchr(url_, '/');
    }
    if (strncasecmp(url_, "https://", 8) == 0){
        url_ += 8;
        url_ = strchr(url_, '/');
    }

    if (!url_ || url_[0] != '/'){
        return BAD_REQUEST;
    }
    if (strlen(url_) == 1){
        strcat(url_, "judge.html");
    }
    check_state_ = CHECK_STATE_HEADER;
    return NO_REQUEST;
}

//解析http请求的一个头部信息
http_conn::HTTP_CODE http_conn::parse_headers(char *text){
    if (text[0] = '\0'){
        if (content_len_ != 0){
            check_state_ = CHECK_STATE_CONTENT;
            return NO_REQUEST;
        }
        return GET_REQUEST;
    }
    else if (strncasecmp(text, "Connection:", 11) == 0){
        text += 11;
        text += strspn(text, " \t");
        if (strcasecmp(text, "keep-alive") == 0){
            linger_ = true;
        }
    }
    else if (strncasecmp(text, "Content-length:", 15) == 0){
        text += 15;
        text += strspn(text, " \t");
        content_len_ = atol(text);
    }
    else if (strncasecmp(text, "Host:", 5) == 0)
    {
        text += 5;
        text += strspn(text, " \t");
        host_ = text;
    }
    else {
        // LOG_INFO("oop!unknow header: %s", text);
    }
    return NO_REQUEST;
}

//判断http请求是否被完整读入
http_conn::HTTP_CODE http_conn::parse_content(char *text){
    if (read_idx_ >= content_len_+checked_idx_){
        text[content_len_] = '\0';
        //POST请求中最后为输入的用户名和密码
        string_ = text;
        return GET_REQUEST;
    }
    return NO_REQUEST;
}

http_conn::HTTP_CODE http_conn::process_read(){
    LINE_STATUS line_status = LINE_OK;
    HTTP_CODE ret = NO_REQUEST;
    char *text = 0;
    while((check_state_ == CHECK_STATE_CONTENT && line_status == LINE_OK) || ((line_status = parse_line()) == LINE_OK)){
        text = get_line();
        start_line_ = checked_idx_;
        // LOG_INFO("%s", text);
        switch (check_state_){
            case CHECK_STATE_REQUESTLINE: 
                ret = parse_request_line(text);
                if (ret == BAD_REQUEST){
                    return BAD_REQUEST;
                }
                break;
            case CHECK_STATE_HEADER:
                ret = parse_headers(text);
                if (ret == BAD_REQUEST){
                    return BAD_REQUEST;
                }
                else if (ret == GET_REQUEST){
                    return do_request();
                }
                break;
            case CHECK_STATE_CONTENT:
                ret = parse_content(text);
                if (ret == GET_REQUEST){
                    return do_request();
                }
                line_status = LINE_OPEN;
                break;
            default:
                return INTERNAL_ERROR;
        }
    }
    return NO_REQUEST;
}