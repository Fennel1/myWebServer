#ifndef CONFIG_H
#define CONFIG_H

#include <unistd.h>
#include <stdlib.h>

class Config{
public:
    Config();
    ~Config() {};

    void parse_arg(int argc, char *argv[]);

    int port_;          //端口号
    int logwrite_;      //日志写入方式
    int et_;            //et模式
    int listenfdMode_;  //listenfd触发模式
    int connfdMode_;    //connfd触发模式
    int linger_;        //关闭链接
    int sqlNum_;        //数据库连接池容量
    int threadNum_;     //线程池线程容量
    int closeLog_;      //日志开关
    int actorModel_;    //并发模式
};

#endif