#ifndef _LOCKER_H_
#define _LOCKER_H_

#include<semaphore.h>
#include<pthread.h>
#include<exception>




class sem
{

 public:
    sem(){
        if(sem_init(&m_sem,0,0)!=0){
            throw std::exception();
        }
    }

    sem(int num){
        if(sem_init(&m_sem,0,num)!=0)
        {
            throw std::exception();
        }
    }

    ~sem()
    {
        sem_destroy(&m_sem);
    }

    bool wait()
    {
        return sem_wait(&m_sem) == 0;
    }

    bool post()
    {
        return sem_post(&m_sem) == 0;
    }

 private:
    sem_t m_sem;
};



class locker
{
 public:

    locker(/* args */){
        if(pthread_mutex_init(&m_mutex,nullptr) != 0){
            throw std::exception();
        }
    }



    ~locker(){
        pthread_mutex_destroy(&m_mutex);
    }

    bool lock(){
        return pthread_mutex_lock(&m_mutex) == 0;
    }

    bool unlock(){
        return pthread_mutex_unlock(&m_mutex)==0;
    }

    pthread_mutex_t *get(){
        return &m_mutex;
    }

 private:
    pthread_mutex_t m_mutex;
    /* data */
};


class cond
{
    /* data */
 public:
    cond(/* args */){
        if(pthread_cond_init(&m_cond,nullptr)!= 0){
            throw std::exception();
        }
    }

    ~cond(){
        pthread_cond_destroy(&m_cond);
    }
    
    // 让线程进入等待状态，直到其他线程发送 signal() 或 broadcast() 。
    bool wait(pthread_mutex_t* mutex){
        int ret = 0;

        ret = pthread_cond_wait(&m_cond,mutex);

        return ret == 0;
    }
    // 让线程进入 限时等待 状态，如果超时还未收到信号，则返回错误。
    bool timewait(pthread_mutex_t *mutex, struct timespec t){

        int ret = 0;

        ret = pthread_cond_timedwait(&m_cond,mutex,&t);

        return ret == 0;
    }
    //唤醒一个等待的线程
    bool signal(){
        return pthread_cond_signal(&m_cond) == 0;
    }
    // 唤醒所有等待的线程。
    bool broadcast(){
        return pthread_cond_broadcast(&m_cond) == 0;
    }

 private:

    pthread_cond_t m_cond;

};






#endif //_LOCKER_H_