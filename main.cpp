#include "config/config.h"
#include "webserver/webserver.h"

#include <iostream>

int main(int argc, char *argv[]){
    std::string user = "root";
    std::string passwd = "123";
    std::string databasename = "webserver";

    Config config;
    config.parse_arg(argc, argv);

    WebServer server;
    server.init(config.port_, user, passwd, databasename, config.logwrite_, 
                config.linger_, config.et_,  config.sqlNum_,  config.threadNum_, 
                config.closeLog_, config.actorModel_);

    // std::cout << "WebServer init success" << std::endl;
    
    server.log_write();
    // std::cout << "WebServer log_write success" << std::endl;
    server.sql_pool();
    // std::cout << "WebServer sql_pool success" << std::endl;
    server.thread_pool();
    // std::cout << "WebServer thread_pool success" << std::endl;
    server.et();
    // std::cout << "WebServer et success" << std::endl;
    server.eventListen();
    // std::cout << "WebServer eventListen success" << std::endl;
    server.eventLoop();
    // std::cout << "WebServer eventLoop success" << std::endl;

    return 0;
}