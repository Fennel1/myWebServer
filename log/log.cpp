#include "log.h"

log::log(){
    count_ = 0;
    is_async_ = false;
}

log::~log(){
    if (fp_ != nullptr){
        fclose(fp_);
    }
}

//异步需要设置阻塞队列的长度，同步不需要设置
bool log::init(const char *file_name, int close_log, int log_buf_size, int split_lines, int max_queue_size){
    if (max_queue_size >= 1){
        is_async_ = true;
        log_queue_ = new block_queue<std::string>(max_queue_size);
        pthread_t tid;
        pthread_create(&tid, nullptr, flush_log_thread, nullptr);
    }

    close_log_ = close_log;
    log_buf_size_ = log_buf_size;
    buf_ = new char[log_buf_size_];
    memset(buf_, '\0', log_buf_size_);
    split_lines_ = split_lines;

    time_t t = time(NULL);
    struct tm *sys_tm = localtime(&t);
    struct tm my_tm = *sys_tm;
    today_ = my_tm.tm_mday;

    const char *p = strrchr(file_name, '/');
    char log_full_name[256] = {0};

    if (p == nullptr){
        snprintf(log_full_name, 255, "%d_%02d_%02d_%s.txt", my_tm.tm_year + 1900, my_tm.tm_mon + 1, my_tm.tm_mday, file_name);
    }
    else{
        strcpy(log_name_, p + 1);
        strncpy(dir_name_, file_name, p - file_name + 1);
        snprintf(log_full_name, 255, "%s%d_%02d_%02d_%s.txt", dir_name_, my_tm.tm_year + 1900, my_tm.tm_mon + 1, my_tm.tm_mday, log_name_);
    }
    
    fp_ = fopen(log_full_name, "a");
    if (fp_){
        return true;
    }
    else{
        return false;
    }
}

void log::write_log(int level, const char *format, ...){
    struct timeval now = {0, 0};
    gettimeofday(&now, NULL);
    time_t t = now.tv_sec;
    struct tm *sys_tm = localtime(&t);
    struct tm my_tm = *sys_tm;

    char s[16] = {0};
    switch (level){
        case 0:
            strcpy(s, "[debug]:");
            break;
        case 1:
            strcpy(s, "[info]:");
            break;
        case 2:
            strcpy(s, "[warn]:");
            break;
        case 3:
            strcpy(s, "[error]:");
            break;
        default:
            strcpy(s, "[debug]:");
            break;
    }

    mutex_.lock();

    count_++;
    if (today_ != my_tm.tm_mday || count_%split_lines_ == 0){
        char new_log[256] = {0};
        fflush(fp_);
        fclose(fp_);

        char tail[16] = {0};
        snprintf(tail, 16, "%d_%02d_%02d_", my_tm.tm_year + 1900, my_tm.tm_mon + 1, my_tm.tm_mday);

        if (today_ != my_tm.tm_mday){
            snprintf(new_log, 255, "%s%s%s", dir_name_, tail, log_name_);
            today_ = my_tm.tm_mday;
            count_ = 0;
        }
        else{
            snprintf(new_log, 255, "%s%s%s.%lld", dir_name_, tail, log_name_, count_/split_lines_);
        }
        fp_ = fopen(new_log, "a");
    }

    mutex_.unlock();

    va_list valst;
    va_start(valst, format);
    std::string log_str;

    mutex_.lock();

    int n = snprintf(buf_, 48, "%d-%02d-%02d %02d:%02d:%02d.%06ld %s ",
                     my_tm.tm_year + 1900, my_tm.tm_mon + 1, my_tm.tm_mday,
                     my_tm.tm_hour, my_tm.tm_min, my_tm.tm_sec, now.tv_usec, s);
    
    int m = vsnprintf(buf_ + n, log_buf_size_ - n - 1, format, valst);
    buf_[n + m] = '\n';
    buf_[n + m + 1] = '\0';
    log_str = buf_;

    mutex_.unlock();

    if (is_async_ && !log_queue_->full()){  //异步写入
        log_queue_->push(log_str);
    }
    else{        //同步写入
        mutex_.lock();
        fputs(log_str.c_str(), fp_);
        mutex_.unlock();
    }

    va_end(valst);
}

void log::flush(void)
{
    mutex_.lock();
    //强制刷新写入流缓冲区
    fflush(fp_);
    mutex_.unlock();
}