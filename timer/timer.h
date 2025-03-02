#ifndef _TIMER_
#define _TIMER_

#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <assert.h>
#include <sys/stat.h>
#include <string.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <stdarg.h>
#include <errno.h>
#include <sys/wait.h>
#include <sys/uio.h>

#include <time.h>
// #include "../log/log.h"

class util_timer;

struct client_data
{
    sockaddr_in address;
    int sockfd;
    util_timer *timer;
};

// 定时器类
class util_timer
{
    /* data */
public:
    util_timer() : pre(nullptr), next(nullptr) {}
    ~util_timer() {}


    // 超时时间
    time_t expire;

    // 回调函数
    void (*call_backFunc)(client_data *);

    // 连接资源
    client_data *user_data;
    // 向前定时器
    util_timer *pre;
    // 向后定时器
    util_timer *next;
};
void call_backFunc(client_data *user_data);


class sort_timer_list
{
    /* data */
 public:
    sort_timer_list():head(nullptr),tail(nullptr){}
    ~sort_timer_list();
    void add_timer(util_timer*timer);

    void adjust_timer(util_timer *timer);

    void del_timer(util_timer*timer);
    void task();

 private:
    void add_timer(util_timer *timer, util_timer *cur_head);
    util_timer* head;
    util_timer* tail;


};







class Util
{
    /* data */
 public:
    Util(/* args */){}
    ~Util(){}
    void init(int timeslot);

    static void sig_handler(int sig);

    void addsig(int sig,void(handler)(int), bool restart = true);

    void timer_handler();

    void show_error(int connfd,const char* info);

 public:
    sort_timer_list m_timer_list;//timer最小堆
    static int m_util_epollfd;//epoll文件描述符
    static int *m_pipefd;//写通道
    int m_TIMESLOT;//定时时间

};




#endif








