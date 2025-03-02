#include "timer.h"
#include "../FuncOfepoll.h"
#include "../http/http_conn.h"

int *Util::m_pipefd = 0;
int Util::m_util_epollfd = 0;

void call_backFunc(client_data *user_data)
{
    if (nullptr == user_data)
        return;
    epoll_ctl(Util::m_util_epollfd, EPOLL_CTL_DEL, user_data->sockfd, 0);
    // 为什么需要断言一下？？？？
    assert(user_data);
    close(user_data->sockfd);
    http_conn::m_user_count--;
}

void Util::init(int timeslot)
{
    m_TIMESLOT = timeslot;
}

// ?????????? 为什么需要保留原来的errno？
void Util::sig_handler(int sig)
{
    int save_errno = errno;
    int msg = sig;
    send(m_pipefd[1], (char *)&msg, 1, 0);
    errno = save_errno;
}

void Util::addsig(int sig, void(handler)(int), bool restart)
{
    struct sigaction sa; // 这个结构体在什么库中？
    memset(&sa, '\0', sizeof(sa));

    sa.sa_handler = handler;

    if (restart)
    {
        sa.sa_flags |= SA_RESTART;
    }

    sigfillset(&sa.sa_mask);
    assert(sigaction(sig, &sa, nullptr) != -1);
}

void Util::timer_handler()
{
    m_timer_list.task();
    alarm(m_TIMESLOT);
}

// 为什么是用connfd发送info？然后关闭连接？
void Util::show_error(int connfd, const char *info)
{
    send(connfd, info, strlen(info), 0);
    close(connfd);
}

// sort_timer_list::~sort_timer_list()
// {
//     auto temp = head;
//     while (temp)
//     {
//         head = head->next;
//         delete temp;
//         temp = head;
//     }
// }

// void sort_timer_list::add_timer(util_timer *timer)
// {

//     if (nullptr == timer)
//     {
//         return;
//     }
//     if (!head)
//     {
//         head = tail = timer;
//         return;
//     }

//     if (timer->expire < head->expire)
//     {
//         head->pre = timer;
//         timer->next = head;
//         head = timer;
//         return;
//     }

//     add_timer(timer, head);
// }

// void sort_timer_list::adjust_timer(util_timer *timer)
// {
//     if (nullptr == timer)
//     {
//         return;
//     }

//     // util_timer * temp = timer->next;

//     if (nullptr == timer->next || timer->expire < timer->next->expire)
//     {
//         return;
//     }

//     if (timer == head)
//     {
//         head = timer->next;
//         head->pre = nullptr;
//         timer->next = nullptr; // 如果timer被插入到list的tail，next为空；

//         add_timer(timer, head);
//     }
//     else
//     {

//         timer->pre->next = timer->next;
//         timer->next->pre = timer->pre;

//         add_timer(timer, timer->next);
//     }
// }

// void sort_timer_list::del_timer(util_timer *timer)
// {
//     if (nullptr == timer)
//     {
//         return;
//     }

//     if (timer == head && timer == tail)
//     {
//         delete timer;
//         head = nullptr;
//         tail = nullptr;
//         return;
//     }

//     if (timer == head)
//     {
//         head = timer->next;
//         head->pre = nullptr;
//         delete timer;
//         return;
//     }

//     if (timer == tail)
//     {
//         timer->pre->next = nullptr;
//         delete timer;
//         return;
//     }

//     timer->pre->next = timer->next;
//     timer->next->pre = timer->pre;
//     delete timer;
//     return;
// }
// void sort_timer_list::task()
// {

//     if (nullptr == head)
//     {
//         return;
//     }

//     time_t cur = time(nullptr);
//     util_timer *cur_timer = head;

//     while (cur_timer != nullptr)
//     {

//         if (cur < cur_timer->expire)
//         {
//             break;
//         }

//         cur_timer->call_backFunc(cur_timer->user_data);

//         head = cur_timer->next;
//         if (head != nullptr)
//         {
//             head->pre = nullptr;
//         }

//         delete cur_timer;
//         cur_timer = head;
//     }
// }

// void sort_timer_list::add_timer(util_timer *timer, util_timer *cur_head)
// {
//     util_timer *cur_pre = cur_head;
//     util_timer *cur = cur_pre->next;

//     while (cur)
//     {
//         if (timer->expire < cur->expire)
//         {
//             timer->next = cur;
//             timer->pre = cur_pre;
//             cur->pre = timer;
//             cur_pre->next = timer;
//             break;
//         }
//         cur_pre = cur;
//         cur = cur->next;
//     }

//     if (nullptr == cur)
//     {
//         cur_pre->next = timer;
//         timer->pre = cur_pre;
//         timer->next = nullptr;
//         tail = timer;
//         return;
//     }
// }










sort_timer_list::~sort_timer_list()
{
    util_timer *tmp = head;
    while (tmp)
    {
        head = tmp->next;
        delete tmp;
        tmp = head;
    }
}

void sort_timer_list::add_timer(util_timer *timer)
{
    if (!timer)
    {
        return;
    }
    if (!head)
    {
        head = tail = timer;
        return;
    }
    if (timer->expire < head->expire)
    {
        timer->next = head;
        head->pre = timer;
        head = timer;
        return;
    }
    add_timer(timer, head);
}
void sort_timer_list::adjust_timer(util_timer *timer)
{
    if (!timer)
    {
        return;
    }
    util_timer *tmp = timer->next;
    if (!tmp || (timer->expire < tmp->expire))
    {
        return;
    }
    if (timer == head)
    {
        head = head->next;
        head->pre = nullptr;
        timer->next = nullptr;
        add_timer(timer, head);
    }
    else
    {
        timer->pre->next = timer->next;
        timer->next->pre = timer->pre;
        add_timer(timer, timer->next);
    }
}
void sort_timer_list::del_timer(util_timer *timer)
{
    if (!timer)
    {
        return;
    }
    if ((timer == head) && (timer == tail))
    {
        delete timer;
        head = nullptr;
        tail = nullptr;
        return;
    }
    if (timer == head)
    {
        head = head->next;
        head->pre = nullptr;
        delete timer;
        return;
    }
    if (timer == tail)
    {
        tail = tail->pre;
        tail->next = nullptr;
        delete timer;
        return;
    }
    timer->pre->next = timer->next;
    timer->next->pre = timer->pre;
    delete timer;
}
void sort_timer_list::task()
{
    if (!head)
    {
        return;
    }
    
    time_t cur = time(nullptr);
    util_timer *tmp = head;
    while (tmp)
    {
        if (cur < tmp->expire)
        {
            break;
        }
        tmp->call_backFunc(tmp->user_data);
        head = tmp->next;
        if (head)
        {
            head->pre = nullptr;
        }
        delete tmp;
        tmp = head;
    }
}

void sort_timer_list::add_timer(util_timer *timer, util_timer *lst_head)
{
    util_timer *pre = lst_head;
    util_timer *tmp = pre->next;
    while (tmp)
    {
        if (timer->expire < tmp->expire)
        {
            pre->next = timer;
            timer->next = tmp;
            tmp->pre = timer;
            timer->pre = pre;
            break;
        }
        pre = tmp;
        tmp = tmp->next;
    }
    if (!tmp)
    {
        pre->next = timer;
        timer->pre = pre;
        timer->next = nullptr;
        tail = timer;
    }
}