#include<stdio.h>
#include<list>
#include<mysql/mysql.h>
#include<error.h>
#include<string.h>
#include<iostream>
#include<string>
#include "sql_connection_pool.h"

connection_pool::connection_pool()
{
    m_Freeconn = 0;
    m_Maxconn = 0;
}


connection_pool::~connection_pool()
{
    DestoryPool();

}


MYSQL *connection_pool::GetConnection()
{
    if(!m_Freeconn)
        return NULL;

    lock.lock();
    MYSQL * conn=connlist.front();
    connlist.pop_front();


    m_Curconn++;
    m_Freeconn--;

    lock.unlock();

    return conn;
}


bool connection_pool::ReleaseConnection(MYSQL*conn){


    if(conn==NULL)return false;


    lock.lock();

    connlist.push_back(conn);

    m_Curconn--;
    m_Freeconn++;

    lock.unlock();
    
    reserve.post();

    return true;

}


int connection_pool::GetFreeConn(){
    return this->m_Freeconn;


}



void connection_pool::DestoryPool(){

    lock.lock();
    if(connlist.size())
    for(auto it=connlist.begin();it!=connlist.end();it++){

        MYSQL *conn=*it;
        mysql_close(conn);
    }
    m_Curconn=0;
    m_Maxconn=0;
    connlist.clear();

    lock.unlock();
}


connection_pool *connection_pool::GetInstance(){

    static connection_pool connpool;
    return &connpool;
}


void connection_pool::init(string url, string User, string PassWord, string DataBaseName, int Port, int MaxConn, int close_log){

    m_url=url;
     m_Port=Port;
     m_user=User;
     m_password=PassWord;
     m_DatabaseName=DataBaseName;
     m_close_log=close_log;


    for(int i=0;i<MaxConn;i++){
        MYSQL* conn=nullptr;
        conn=mysql_init(conn);

        if(conn==nullptr){
            printf("Log mysql error");
            exit(1);
        }

        conn=mysql_real_connect(conn,url.c_str(),User.c_str(),PassWord.c_str(),DataBaseName.c_str(),Port,NULL,0);

        if(conn==NULL){
            printf("Log mysql error");
            exit(1);
        }

        connlist.push_back(conn);
        m_Freeconn++;
    }

    reserve=sem(m_Freeconn);
    m_Maxconn=m_Freeconn;
}



//
connectionRAII::connectionRAII(MYSQL**SQL,connection_pool *connpool){//zhe li MYSQl wei sm shi shuang zhizhen??

    *SQL=connpollRAII->GetConnection();
    connRAII=*SQL;
    connpollRAII=connpool;


}


connectionRAII::~connectionRAII(){

    connpollRAII->ReleaseConnection(connRAII);
}

