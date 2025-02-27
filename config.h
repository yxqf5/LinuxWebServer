
#ifndef  _CONFIG_H_
#define _CONFIG_H_

#include"webserver.h"
#include <unistd.h>
#include<stdlib.h>
using namespace std;




class Config
{
private:
    /* data */
public:

    Config(/* args */);
    ~Config();

    //解析命令行参数
    void parse_arg(int argc, char*argv[]);

    //端口号
    int PORT;

    //日志写入方式
    int LOGWrite;

    //触发组合模式
    int TRIGMode;

    //listenfd的触发方式
    int LISTENTrigmode;

    //connfd的触发方式
    int CONNTrigmode;

    //用于关闭连接，暂时还不太明白；
    int OPT_LINGER;


    //数据库连接池中连接的数量
    int sql_num;

    //线程池中线程的数量
    int thread_num;

    //日志是否关闭
    int close_log;


    //并发模型选择
    int actor_model;

};

#endif  //_CONFIG_H_
