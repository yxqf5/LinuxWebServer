
#ifndef _THREADPOOL_H_
#define _THREADPOOL_H_

#include <list>
#include <cstdio>
#include <exception>
#include <pthread.h>
#include "../lock/locker.h"
#include "../sql_connpool/sql_connection_pool.h"


template <typename T>
class threadpool
{

public:
//  int actor_model, connection_pool * connpool,int thread_number=8,int max_request=10000
    threadpool(int actor_model, connection_pool * connpool,int thread_number=8,int max_request=10000);
    ~threadpool();
    bool append(T *request,int state);
    bool append_p(T *request);


private:
    static void *worker(void *arg);
    void run();


private:
    int m_thread_number;        //线程池中的线程数
    int m_max_requests;         //请求队列中允许的最大请求数
    pthread_t *m_threads;       //描述线程池的数组，其大小为m_thread_number
    std::list<T *> m_workqueue; //请求队列
    locker m_queuelocker;       //保护请求队列的互斥锁
    sem m_queuestat;            //是否有任务需要处理
    connection_pool *m_connPool;  //数据库
    int m_actor_model;          //模型切换
};

template <typename T>
threadpool<T>::threadpool(int actor_model, connection_pool * connpool,int thread_number=8,int max_request=10000)
{
    if(thread_number<= 0||max_request<= 0)
        throw std::exception();

    m_threads=new pthread_t[thread_number];
    if (!m_threads)
        throw std::exception();
    

    for(int i=0;i<thread_number;i++){
        
        if(pthread_create(m_threads+i,NULL,worker,this)!=0 )///用来创建线程
        {
            delete[] m_threads;
            throw std::exception();
        }   
        if (pthread_detach(m_threads[i])!= 0)//给线程标记为分离状态，这意味着线程在结束时，会自动进行资源回收
        {
            delete[] m_threads;
            throw std::exception();             
        }
    }
}


template <typename T>
threadpool<T>::~threadpool()
{
    delete m_threads;
}

template <typename T>
bool threadpool<T>::append(T *request, int state){
    m_queuelocker.lock();
    if(m_workqueue.size()>=m_max_requests){
        m_queuelocker.unlock();
        return false;
    }

    request->m_satae=state;
    m_workqueue.push_back(request);
    m_queuelocker.unlock();
    m_queuestat.post();
    return true;
}

template <typename T>
bool threadpool<T>::append_p(T *request){
    
    m_queuelocker.lock();
    if(m_workqueue.size()>=m_max_requests){
        m_queuelocker.unlock();
            return false;
    }
    m_workqueue.push_back(request);
    m_queuelocker.unlock();

    m_queuestat.post();
    return true;
}

template <typename T>
void* threadpool<T>::worker(void *arg){

    threadpool * pool=(threadpool*)arg;
    pool->run();
    return pool;

}






template <typename T>
void threadpool<T>::run(){

    while(true){


        m_queuestat.wait();
        m_queuelocker.lock();
        if(m_workqueue.empty()){
            m_queuelocker.unlock();
            continue;
        }
        
        T* request =m_workqueue.front();
        m_workqueue.pop_front();
        m_queuelocker.unlock();
        if(!request)
            continue;
        if (1 == m_actor_model)
        {
            if (0 == request->m_state)
            {
                if (request->read_once())
                {

                    request->improv = 1;
                    connectionRAII mysqlconn(&request->mysql, m_connPool);
                    request->process();
                }
                else
                {
                    request->improv = 1;
                    request->timer_flag = 1;
                }
            }
            else
            {

                if(request->write()){
                    request->improv=1;

                }
                else{
                    request->improv=1;
                    request->timer_flag=1;
                }
              
            }
        }
        else
        {
            connectionRAII mysqlcon(&request->mysql,m_connPool);
            request->process();

        }
    }


}
////run函数还有很多疑问，等后面写http的时候回顾一下
// template <typename T>
// void threadpool<T>::run()
// {
//     while (true)
//     {
//         m_queuestat.wait();//信号量等待
//         m_queuelocker.lock();
//         if (m_workqueue.empty())
//         {
//             m_queuelocker.unlock();
//             continue;
//         }
//         //request 类型是怎样的，我的疑惑在这里。
//         T *request = m_workqueue.front();//我的问题是，request，既然没有定义成员，那么为什么一会可以request-》m_state,一会又可以是request->mysql,这是怎样实现的，是模板函数的特性吗（模板T可以自由添加成员）？请详细解释！
//         m_workqueue.pop_front();
//         m_queuelocker.unlock();
//         if (!request)
//             continue;
//         if (1 == m_actor_model)
//         {
//             if (0 == request->m_state)
//             {
//                 if (request->read_once())
//                 {
//                     request->improv = 1;
//                     connectionRAII mysqlcon(&request->mysql, m_connPool);
//                     request->process();//为什么还可以调用http中的函数？难道他是http中定义的？？
//                 }
//                 else
//                 {
//                     request->improv = 1;//这里是在干什么？？这个状态是用来干什么的？
//                     request->timer_flag = 1;
//                 }
//             }
//             else
//             {
//                 if (request->write())
//                 {
//                     request->improv = 1;
//                 }
//                 else
//                 {
//                     request->improv = 1;
//                     request->timer_flag = 1;
//                 }
//             }
//         }
//         else
//         {
//             connectionRAII mysqlcon(&request->mysql, m_connPool);
//             request->process();
//         }
//     }
// }




#endif _THREADPOOL_H_