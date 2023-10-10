#ifndef LOG_H
#define LOG_H

#include <string>
#include <string.h>
#include <sys/time.h>
#include <stdarg.h>
#include "block_queue.h"

class log{
public:
    static log* get_instance(){
        static log instance;
        return &instance;
    }

    static void *flush_log_thread(void *args){
        log::get_instance()->async_write_log();
    }

    //可选择的参数有日志文件、日志缓冲区大小、最大行数以及最长日志条队列
    bool init(const char *file_name, int close_log, int log_buf_size = 8192, int split_lines = 5000000, int max_queue_size = 0);
    void write_log(int level, const char *format, ...);
    void flush(void);

private:
    log();
    virtual ~log();
    void *async_write_log(){
        std::string single_log;
        //从阻塞队列中取出一个日志string，写入文件
        while (log_queue_->pop(single_log)){
            mutex_.lock();
            fputs(single_log.c_str(), fp_);
            mutex_.unlock();
        }
    }

    char dir_name_[128];    //路径名
    char log_name_[128];    //log文件名
    int split_lines_;       //日志最大行数
    int log_buf_size_;      //日志缓冲区大小
    long long count_;       //日志行数记录
    int today_;             //因为按天分类,记录当前时间是那一天
    FILE *fp_;              //打开log的文件指针
    char *buf_;
    block_queue<std::string> *log_queue_;   //阻塞队列
    bool is_async_;     //是否同步标志位
    locker mutex_;
    int close_log_;     //关闭日志
};

// #define LOG_DEBUG(format, ...) if(0 == close_log_) {log::get_instance()->write_log(0, format, ##__VA_ARGS__); log::get_instance()->flush();}
// #define LOG_INFO(format, ...) if(0 == close_log_) {log::get_instance()->write_log(1, format, ##__VA_ARGS__); log::get_instance()->flush();}
// #define LOG_WARN(format, ...) if(0 == close_log_) {log::get_instance()->write_log(2, format, ##__VA_ARGS__); log::get_instance()->flush();}
// #define LOG_ERROR(format, ...) if(0 == close_log_) {log::get_instance()->write_log(3, format, ##__VA_ARGS__); log::get_instance()->flush();}

#define LOG_DEBUG(format, ...)  {log::get_instance()->write_log(0, format, ##__VA_ARGS__); log::get_instance()->flush();}
#define LOG_INFO(format, ...)  {log::get_instance()->write_log(1, format, ##__VA_ARGS__); log::get_instance()->flush();}
#define LOG_WARN(format, ...)  {log::get_instance()->write_log(2, format, ##__VA_ARGS__); log::get_instance()->flush();}
#define LOG_ERROR(format, ...)  {log::get_instance()->write_log(3, format, ##__VA_ARGS__); log::get_instance()->flush();}

#endif