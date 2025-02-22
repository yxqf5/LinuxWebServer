#ifndef HTTPCONNECTION_H
#define HTTPCONNECION_H

#include <unistd.h>//close()
// #include <signal.h>//?
// #include <sys/types.h>//?
#include <sys/epoll.h>
#include <fcntl.h>//open()
#include <sys/socket.h>
#include <netinet/in.h>//struct sockaddr_in 
// #include <arpa/inet.h>
#include <assert.h> //duan yan
#include <sys/stat.h>//struct stat   -n 165 
#include <string.h>
#include <pthread.h>
#include <stdio.h>
// #include <stdlib.h>
#include <sys/mman.h>//zhe ge shi bu shi na ge [mmap();] function;
// #include <stdarg.h>
#include <errno.h>
// #include <sys/wait.h>
#include <sys/uio.h>//writev()
#include <map>
#include<string>

#include "../lock/locker.h"
#include "../sql_connpool/sql_connection_pool.h"
// #include "../timer/lst_timer.h"
// #include "../log/log.h"



class http_conn
{
 public:
    static const int FILENAME_LEN = 200;
    static const int READ_BUFFER_SIZE = 2048;
    static const int WRITE_BUFFER_SIZE = 1024;


    // GET POST ....
    enum METHOD
    {
        GET = 0,
        POST,
        HEAD,
        PUT,
        DELETE,
        TRACE,
        OPTIONS,
        CONNECT,
        PATH
    };

    //
    enum CHECK_STATE
    {
        CHECK_STATE_REQUESTLINE = 0,
        CHECK_STATE_HEADER,
        CHECK_STATE_CONTENT
    };

    enum LINE_STATUS
    {
        LINE_OK = 0,
        LINE_BAD,
        LINE_OPEN
    };

    enum HTTP_CODE
    {
        NO_REQUEST,
        GET_REQUEST,
        BAD_REQUEST,
        NO_RESOURCE,
        FORBIDDEN_REQUEST,
        FILE_REQUEST,
        INTERNAL_ERROR,
        CLOSED_CONNECTION
    };


 public:
    http_conn(){}
    ~http_conn(){}

 public:
    void init(int sockfd,const sockaddr_in &addr,char*,int,int,string user,string passwd,string sqlname);
    void close_conn(bool real_close =true);
    void process();
    bool read_once();
    bool write();

    sockaddr_in * get_address(){
        return &m_address;
    }

    void initmysql_result(connection_pool * connpool);

    int timer_flag;//?
    int improv;//?




 private:
    void init();
    HTTP_CODE process_read();
    bool process_write(HTTP_CODE ret);

    HTTP_CODE parse_request_line(char* text);
    HTTP_CODE parse_headers(char*text);
    HTTP_CODE parse_content(char*text);
    HTTP_CODE do_request();

    char* get_line(){return m_read_buf + m_start_line;};

    LINE_STATUS parse_line();
    void unmap();


    //
    //响应报文格式，以下函数均由do——request调用。
    bool add_response(const char* format,...);//canshu yet
    bool add_status_line(int status,const char*title);
    
    //headers
    bool add_headers(int content_length);
    bool add_content_type();
    bool add_content_length(int content_length);
    bool add_linger();
    bool add_blank_line();

    //content
    bool add_content(const char* content);

 public:
    static int m_epollfd;
    static int m_user_count;//? what? what is this?
    MYSQL *mysql;
    int m_state;


 private:
    int m_sockfd;
    sockaddr_in m_address;

    char m_read_buf[READ_BUFFER_SIZE];
    int m_read_idx;
    int m_checked_idx;
    int m_start_line;

    char m_write_buf[WRITE_BUFFER_SIZE];
    int m_write_idx;

    CHECK_STATE m_check_state;
    METHOD m_method;


    //存放文件名
    char m_real_file[FILENAME_LEN];

    char* m_url;
    char* m_version;
    char* m_host;

    int m_content_length;
    bool m_linger;// connection : keep-alive

    char* m_file_address;
    struct stat m_file_stat;
    struct iovec m_iv[2];

    int m_iv_count;
    int m_cgi;
    char *m_string;//存储conten中的用户名和密码；

    int bytes_to_send;
    int bytes_have_send;
    char *doc_root; // 根目录

    map<string,string>m_users;
    int m_TRIGMode;
    int m_close_log;

    char sql_user[100];
    char sql_passwd[100];
    char sql_name[100];


};





#endif // HTTPCONNECION_H