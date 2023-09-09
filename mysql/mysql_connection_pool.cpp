#include "mysql_connection_pool.h"

connection_pool::connection_pool(){
    this->free_conn_ = 0;
}

connection_pool::~connection_pool()
{
	DestroyPool();
}

connection_pool *connection_pool::GetInstance()
{
	static connection_pool connPool;
	return &connPool;
}

void connection_pool::init(string url, string User, string PassWord, string DataBaseName, int Port, int MaxConn, bool close_log){
    url_ = url;
    port_ = Port;
    user_ = User;
    password_ = PassWord;
    database_name_ = DataBaseName;
    close_log_ = close_log;

    for (int i=0; i<MaxConn; i++){
        MYSQL *con = nullptr;

        con = mysql_init(con);
        if (con == nullptr){
            // LOG_ERROR("MySQL Error");
            exit(1);
        }

        con = mysql_real_connect(con, url.c_str(), user.c_str(), password.c_str(), database_name.c_str(), port, nullptr, 0);
        if (con == nullptr){
            // LOG_ERROR("MySQL Error");
            exit(1);
        }
        conn_list_.push_back(con);
        free_conn_++;
    }

    reserve_ = sem(free_conn_);
    max_conn_ = free_conn_;
}

MYSQL *connection_pool::GetConnection(){
    if (free_conn_ == 0){
        return nullptr;
    }

    MYSQL *con = nullptr;
    reserve_.wait();
    lock_.lock();

    con = conn_list_.front();
    conn_list_.pop_front();
    --free_conn_;

    lock_.unlock();
    return con;
}

bool connection_pool::ReleaseConnection(MYSQL *conn){
    if (conn == nullptr){
        return false;
    }

    lock_.lock();

    conn_list_.push_back(conn);
    ++free_conn_;

    lock_.unlock();
    reserve_.post();
    return true;
}

void connection_pool::DestroyPool(){
    lock_.lock();

    if (free_conn_ > 0){
        for (auto it=conn_list_.begin(); it!=conn_list_.end(); ++it){
            MYSQL *con = *it;
            mysql_close(con);
        }
        free_conn_ = 0;
        conn_list_.clear();
    }

    lock_.unlock();
}

int connection_pool::GetFreeConnNum(){
    return this->free_conn_;
}



connectionRAII::connectionRAII(MYSQL **sql, connection_pool *connPool){
    *sql = connPool->GetConnection();
    conRAII_ = *sql;
    poolRAII_ = connPool;
}

connectionRAII::~connectionRAII(){
    poolRAII_->ReleaseConnection(conRAII_);
}