#include "config.h"

Config::Config(/* args */)
{

    // 默认端口是9500
    PORT = 9500;

    // 日志写入方式默认为同步；
    LOGWrite = 0;

    // 触发组合模式 默认为：listenfd LT +  connfd LT
    TRIGMode = 0;

    // listenfd默认触发方式为 LT
    LISTENTrigmode = 0;

    // connfd的默认触发方式为LT
    CONNTrigmode = 0;

    // 关闭连接方式 默认关闭，不清除用来干什么的；
    OPT_LINGER = 0;

    // 数据库连接数量默认为8
    sql_num = 8;

    // 线程池线程数量默认为8
    thread_num = 8;

    // 日志，默认不关闭；
    close_log = 0;

    // 并发模式  默认为proactor；
    actor_model = 0;
}

Config::~Config(){}

void Config::parse_arg(int argc, char *argv[])
{

    // optarg是一个getopt中的一个全局变量，用来储存当前选项的值；
    int opt;
    const char *optstr = "p:l:m:o:s:t:c:a:";
    while ((opt = getopt(argc, argv, optstr)) != -1)
    {

        switch (opt)
        {

        case 'p':
        {
            PORT = atoi(optarg);
            break;
        }
        case 'l':
        {
            LOGWrite = atoi(optarg);
            break;
        }
        case 'm':
        {
            TRIGMode = atoi(optarg);
            break;
        }
        case 'o':
        {
            OPT_LINGER = atoi(optarg);
            break;
        }
        case 's':
        {
            sql_num = atoi(optarg);
            break;
        }
        case 't':
        {
            thread_num = atoi(optarg);
            break;
        }
        case 'c':
        {
            close_log = atoi(optarg);
            break;
        }
        case 'a':
        {
            actor_model = atoi(optarg);
            break;
        }
        default:
            break;
        }
    }
}
