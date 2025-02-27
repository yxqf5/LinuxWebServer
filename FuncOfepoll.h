
#ifndef _FUNC_EPOLL_
#define _FUNC_EPOLL_


// 这个将文件描述符设置为非阻塞态，主要是用于epoll的ET 模式； 但是函数实现用的相关宏和方法不太理解
 inline int setnonblocking(int fd)
{
    int old_option = fcntl(fd, F_GETFL);
    int new_option = old_option | O_NONBLOCK;
    fcntl(fd, F_SETFL, new_option);
    return old_option;
}


inline void addfd(int epollfd, int fd, bool one_shot, int TRIFMode)
{
    epoll_event event;
    event.data.fd = fd;
    // ET 模式   EPOLLONESHOT,EPOLLRDHUP是什么意义？？
    if (TRIFMode == 1)
    {
        event.events = EPOLLIN | EPOLLET | EPOLLRDHUP;
    }
    else
    {
        event.events = EPOLLIN | EPOLLRDHUP;
    }

    if (one_shot)
    {
        event.events |= EPOLLONESHOT;
    }

    epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &event);
    setnonblocking(fd);
}

inline void removefd(int epollfd, int fd)
{
    epoll_ctl(epollfd, EPOLL_CTL_DEL, fd, 0);
    close(fd);
}

inline void modfd(int epollfd, int fd, int ev, int TRIGMode)
{
    epoll_event event;
    event.data.fd = fd;

    if (TRIGMode == 1)
    {
        event.events = ev | EPOLLONESHOT | EPOLLET | EPOLLRDHUP;
    }
    else
    {
        event.events = ev | EPOLLONESHOT | EPOLLRDHUP;
    }

    epoll_ctl(epollfd, EPOLL_CTL_MOD, fd, &event);
}


#endif //_FUNC_EPOLL_