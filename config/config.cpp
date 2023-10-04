#include "config.h"

Config::Config(){
    port_ = 6521;
    logwrite_ = 0;      //日志写入，默认同步
    et_ = listenfdMode_ = connfdMode_ = 0; //默认listenfd LT + connfd LT
    linger_ = 0;
    sqlNum_ = 8;
    threadNum_ = 8;
    closeLog_ = 0;      //默认打开日志
    actorModel_ = 0;    //默认为proactor
}

void Config::parse_arg(int argc, char*argv[]){
    int opt;
    const char *str = "p:l:m:o:s:t:c:a:";
    while((opt = getopt(argc, argv, str)) != -1){
        switch (opt){
            case 'p':
                port_ = atoi(optarg);
                break;
            case 'l':
                logwrite_ = atoi(optarg);
                break;
            case 'm':
                et_ = atoi(optarg);
                break;
            case 'o':
                linger_ = atoi(optarg);
                break;
            case 's':
                sqlNum_ = atoi(optarg);
                break;
            case 't':
                threadNum_ = atoi(optarg);
                break;
            case 'c':
                closeLog_ = atoi(optarg);
                break;
            case 'a':
                actorModel_ = atoi(optarg);
                break;
            default:
                break;
        }
    }
}