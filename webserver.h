#ifndef _WEB_SERVER_
#define _WEB_SERVER_


#include <sys/socket.h>
// #include <netinet/in.h>
#include <arpa/inet.h>//inet_ntoa

#include <stdio.h>
// #include <unistd.h>
#include <errno.h>
// #include <fcntl.h> //
#include <stdlib.h>
#include <cassert>
#include <sys/epoll.h>

#include "http/http_conn.h"
#include "threadpool/threadpool.h"



const int MAX_FD = 65336;
const int MAX_EVENT_NUMBER = 10000;
// const int TIMESLOT = 5;  //最小超时单位



//webserver对象会创建几个？？ 具体怎么使用的？？
class WebServer
{
private:
    /* data */
    int m_port;
    char *m_root;
    
    int m_log_write;
    int m_close_log;
    int m_actormodel;

    int m_pipefd[2];//?
    int m_epollfd;
    http_conn *users;


    //数据库连接池
    connection_pool *m_connPool;
    string m_user;
    string m_passwd;
    string m_databaseName;
    int m_sql_num;

    //线程池
    threadpool<http_conn> *m_pool;  //模板参数在这里添加的
    int m_thread_num;

    //epoll_event 相关部分
    epoll_event events[MAX_EVENT_NUMBER];

    int m_listenfd;
    int m_OPT_LINGER;
    int m_TRIGMode;
    int m_LISTENTrigmode;
    int m_CONNTrigmode;


    //定时器
    //

public:
    WebServer(/* args */);
    ~WebServer();


    void init(int port , string user, string passWord, string databaseName,
              int log_write , int opt_linger, int trigmode, int sql_num,
              int thread_num, int close_log, int actor_model);
    void threal_pool();
    void sql_pool();
    // void log_write();
    void trig_mode();
    void eventListen();
    void eventLoop();
    void timer(int connfd, struct sockaddr_in client_address);
    // void adjust_timer();

    // void deal_timer();
    bool dealclinetdata();
    // bool dealwithsignal();
    
    void dealwithread(int sockfd);
    void dealwithwrite(int sockfd);
};


#endif //_WEB_SERVER_





