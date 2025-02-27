#include"config.h"
// #include<string>

using std::string;


int main(int argc, char* argv[]){

    string user = "root";
    string passwd = "root";
    string dataname = "LinuxWebServer";
    

    Config config;
    config.parse_arg(argc,argv);


    WebServer server;


    server.init(config.PORT, user, passwd, dataname, config.LOGWrite, 
                config.OPT_LINGER,config.TRIGMode, config.sql_num, 
                config.thread_num, config.close_log, config.actor_model);

    
    //server.log_write();


    server.sql_pool();

    server.threal_pool();

    server.trig_mode();

    server.eventListen();

    server.eventLoop();


    return 0;
}