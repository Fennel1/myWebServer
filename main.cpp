#include "config/config.h"
#include "webserver/webserver.h"

int main(int argc, char *argv[]){
    std::string user = "root";
    std::string passwd = "";
    std::string databasename = "webserver";

    Config config;
    config.parse_arg(argc, argv);

    WebServer server;
    server.init(config.port_, user, passwd, databasename, config.logwrite_, 
                config.linger_, config.et_,  config.sqlNum_,  config.threadNum_, 
                config.closeLog_, config.actorModel_);
    
    server.log_write();
    server.sql_pool();
    server.thread_pool();
    server.et();
    server.eventListen();
    server.eventLoop();

    return 0;
}