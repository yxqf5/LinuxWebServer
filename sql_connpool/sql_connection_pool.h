
// #ifdef  _CONNECTION_POOL_
// #define _CONNECTION_POOL_

#pragma once

#include<stdio.h>
#include<list>
#include<mysql/mysql.h>
#include<error.h>
#include<string.h>
#include<iostream>
#include<string>
using std::string;

//lock
#include"../lock/locker.h"
//#include"../log/log.h"



using namespace std;


class connection_pool
{
    public:
    MYSQL * GetConnection();
    bool ReleaseConnection(MYSQL*conn);
    int GetFreeConn();
    void DestoryPool();



    static connection_pool*GetInstance();

    void init(string url, string User, string PassWord, string DataBaseName, int Port, int MaxConn, int close_log);


    private:

        connection_pool();
        ~connection_pool();

        int m_Maxconn;//最大连接数
        int m_Curconn;//当前已使用的连接数
        int m_Freeconn;//当前空闲的连接数
        locker lock;//locker.h
        list<MYSQL*>connlist;
        //PV
        sem reserve;

    public:

    string m_url;
    string m_Port;
    string m_user;
    string m_password;
    string m_DatabaseName;
    int m_close_log;
};



class connectionRAII
{

public:
    connectionRAII(MYSQL**conn,connection_pool *connpool);
    ~connectionRAII();


private:


    connection_pool *connpollRAII;
    MYSQL *connRAII;

};


// #endif _CONNECTION_POOL_
