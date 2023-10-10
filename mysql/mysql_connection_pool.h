#ifndef MYSQL_CONNECTION_POOL_H
#define MYSQL_CONNECTION_POOL_H

#include <mysql/mysql.h>
#include <list>
#include <string>
#include "../lock/locker.h"
#include "../log/log.h"

class connection_pool{
public:
    MYSQL *GetConnection();
    bool ReleaseConnection(MYSQL *conn);
    int GetFreeConnNum();
    void DestroyPool();

    static connection_pool *GetInstance();
    void init(std::string url, std::string User, std::string PassWord, std::string DataBaseName, int Port, int MaxConn, bool close_log);

private:
    connection_pool();
	~connection_pool();

    int max_conn_;
    int free_conn_;
    locker lock_;
    std::list<MYSQL *> conn_list_;
    sem reserve_;

public:
    std::string url_;
    std::string port_;
    std::string user_;
    std::string password_;
    std::string database_name_;
    bool close_log_;
};


class connectionRAII{
public:
    connectionRAII(MYSQL **sql, connection_pool *connPool);
    ~connectionRAII();

private:
    MYSQL *conRAII_;
    connection_pool *poolRAII_;
};

#endif