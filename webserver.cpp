
#include "webserver.h"
#include "FuncOfepoll.h"


WebServer::WebServer(/* args */)
{

    users = new http_conn[MAX_FD];

    // root目录

    char server_path[200];
    getcwd(server_path, 200);

    char root[] = "/root";

    // 注意看，这块内存有没有释放
    m_root = (char *)malloc(strlen(server_path) + strlen(root) + 1);

    strcpy(m_root, server_path);
    strcat(m_root, root);

    // timer
}
WebServer::~WebServer()
{

    close(m_epollfd);
    close(m_listenfd);
    close(m_pipefd[0]); // 这个是什么？
    close(m_pipefd[1]);

    delete[] users;
    // delete[] ;
    delete m_pool;
}

void WebServer::init(int port, string user, string passWord, string databaseName,
                     int log_write, int opt_linger, int trigmode, int sql_num,
                     int thread_num, int close_log, int actor_model)
{
    // 数据库
    m_port = port;
    m_user = user;
    m_passwd = passWord;
    m_databaseName = databaseName;
    m_sql_num = sql_num;

    // 日志
    m_log_write = log_write;
    m_close_log = close_log;

    // 模式选择
    m_TRIGMode = trigmode;
    m_actormodel = actor_model;

    m_thread_num = thread_num;
    // 是否长连接
    m_OPT_LINGER = opt_linger;
}

void WebServer::trig_mode()
{

    if (m_TRIGMode == 0)
    {
        m_LISTENTrigmode = 0;
        m_CONNTrigmode = 0;
    }

    else if (m_TRIGMode == 1)
    {
        m_LISTENTrigmode = 0;
        m_CONNTrigmode = 1;
    }

    else if (m_TRIGMode == 2)
    {
        m_LISTENTrigmode = 1;
        m_CONNTrigmode = 0;
    }
    else if (m_TRIGMode == 3)
    {
        m_LISTENTrigmode = 1;
        m_CONNTrigmode = 1;
    }
}

void WebServer::threal_pool()
{

    m_pool = new threadpool<http_conn>(m_actormodel, m_connPool, m_thread_num);
}
void WebServer::sql_pool()
{
    m_connPool = connection_pool::GetInstance();
    m_connPool->init("localhost", m_user, m_passwd, m_databaseName, 3306, m_sql_num, m_close_log);
    users->initmysql_result(m_connPool);
}
// void WebServer::log_write();

void WebServer::eventListen()
{
    m_listenfd = socket(PF_INET, SOCK_STREAM, 0);
    assert(m_listenfd >= 0);

    // close() OR shoutdown()  to close connection;
    if (m_OPT_LINGER == 0)
    {
        struct linger temp = {0, 1};
        setsockopt(m_listenfd, SOL_SOCKET, SO_LINGER, &temp, sizeof(temp));
    }
    else if (m_OPT_LINGER == 1)
    {
        struct linger temp = {1, 1};
        setsockopt(m_listenfd, SOL_SOCKET, SO_LINGER, &temp, sizeof(temp));
    }

    struct sockaddr_in address;

    bzero(&address, sizeof(address)); //??
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_ANY);
    address.sin_port = htons(m_port);

    int ret = 0;
    int flag = 1;

    setsockopt(m_listenfd, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(flag));
    ret = bind(m_listenfd, (struct sockaddr *)&address, sizeof(address));

    assert(ret >= 0);

    ret = listen(m_listenfd, 5);
    assert(ret >= 0);

    // 计时器没写；

    // epoll创建

    epoll_event events[MAX_EVENT_NUMBER];
    m_epollfd = epoll_create(5);
    assert(m_epollfd != -1);

    addfd(m_epollfd, m_listenfd, false, m_LISTENTrigmode);
    http_conn::m_epollfd = m_epollfd;

    // 后面都和定时器，信号量有关
    ret = socketpair(PF_UNIX, SOCK_STREAM, 0, m_pipefd);
    assert(ret != -1);
    setnonblocking(m_pipefd[1]);
    addfd(m_epollfd, m_pipefd[0], false, 0);
}

// socket 创建
void WebServer::eventLoop()
{

   //printf("the eventLoop function \n");
    bool timeout = false;

    bool stop_server = false;

    while (!stop_server)
    {

        int number = epoll_wait(m_epollfd, events, MAX_EVENT_NUMBER, -1);

        //printf("epoll_wait number = %d \n",number);
        if (number < 0 && errno != EINTR)
        {
            break;
        }

        for (int i = 0; i < number; i++)
        {

            int sockfd = events[i].data.fd;

            if (sockfd == m_listenfd)
            {

                // 我有一个疑问，就是listenfd是怎样重新加入epoll监控中的？
                // m_listenfd 没有使用EPOLLONESHOT ，所以一直会被epoll检测
                bool flag = dealclinetdata();


               // printf("// m_listenfd 没有使用EPOLLONESHOT ，所以一直会被epoll检测");
                if (flag == false)
                    continue;
            }
            else if (events[i].events & (EPOLLRDHUP | EPOLLHUP | EPOLLERR))
            {
                epoll_ctl(m_epollfd, EPOLL_CTL_DEL, sockfd, 0);
                close(sockfd);
                http_conn::m_user_count--;

                /* code */
            }
            // 处理读写
            else if (events[i].events & EPOLLIN)
            {

                dealwithread(sockfd);
            }
            
            else if (events[i].events & EPOLLOUT)
            {
                dealwithwrite(sockfd);
            }
        }
    }
}
void WebServer::timer(int connfd, struct sockaddr_in client_address)
{

    users[connfd].init(connfd, client_address, m_root, m_CONNTrigmode, m_close_log, m_user, m_passwd, m_databaseName);
}
// void WebServer::adjust_timer();

// void WebServer::deal_timer();
bool WebServer::dealclinetdata()
{

    struct sockaddr_in client_address;
    socklen_t client_addrlength = sizeof(client_address);

    auto handle_connection = [&]() -> bool
    {
        int connfd = accept(m_listenfd, (struct sockaddr *)&client_address, &client_addrlength);

        if (connfd < 0)
        {
            //printf(" accept errno is:%d", errno);
            return false;
        }

        if (http_conn::m_user_count >= MAX_FD)
        {
            //printf("Internal server busy");
            return false;
        }

        timer(connfd, client_address);

        return true;
    };

    if (m_LISTENTrigmode == 0)
    {
        return handle_connection();
    }
    else
    {
        while (handle_connection())
        {
        }
        return false;
    }
}
// bool WebServer::dealwithsignal();

void WebServer::dealwithread(int sockfd)
{

    // reator  //未完成
    if (m_actormodel == 1)
    {

        

        m_pool->append(users + sockfd, 0);
    }
    else
    {

        // proactor
        if (users[sockfd].read_once())
        {
            m_pool->append_p(users + sockfd);
        }
        else
        {
            epoll_ctl(m_epollfd, EPOLL_CTL_DEL, sockfd, 0);
            close(sockfd);
            http_conn::m_user_count--;
        }
    }
}
void WebServer::dealwithwrite(int sockfd)
{

    if (m_actormodel == 1)
    {

        m_pool->append(users + sockfd, 1);
    }

    else
    {

        if (users[sockfd].write())
        {
           // printf("send data to the client(%s)", inet_ntoa(users[sockfd].get_address()->sin_addr));
        }
        else{
                            epoll_ctl(m_epollfd, EPOLL_CTL_DEL, sockfd, 0);
                close(sockfd);
                http_conn::m_user_count--;
        }
    }
}

