
#include "http_conn.h"

#include <mysql/mysql.h>
#include <fstream>//open()

using std::string;

//http_state_responses;
const char *ok_200_title = "OK";
const char *error_400_title = "Bad Request";
const char *error_400_form = "Your request has bad syntax or is inherently impossible to staisfy.\n";
const char *error_403_title = "Forbidden";
const char *error_403_form = "You do not have permission to get file form this server.\n";
const char *error_404_title = "Not Found";
const char *error_404_form = "The requested file was not found on this server.\n";
const char *error_500_title = "Internal Error";
const char *error_500_form = "There was an unusual problem serving the request file.\n";

locker m_lock;
map<string,string>users;

int http_conn::m_user_count=0;
int http_conn::m_epollfd=-1;




//用来取消内存映射
void http_conn::unmap()
{
    if(m_file_address){
        munmap(m_file_address, m_file_stat.st_size);
        m_file_address = 0;
        
    }
}


//http处理过程，先是调用process_read()解析,然后调用process_write()编写响应报文
//如果报文获取不完整，则将相应的socketfd再次加入epoll中监控，继续接受剩余报文；
//处理解析完成请求报文，并完成响应报文的编写之后，会向相应的socket触发为写出事件，以通知主线程调用writev进行IO操作；
void http_conn::process()
{
    HTTP_CODE read_ret = process_read();
    //printf("process read_ret");
    if (read_ret == NO_REQUEST)
    {
        modfd(m_epollfd, m_sockfd, EPOLLIN, m_TRIGMode);
        return;
    }

    bool write_ret = process_write(read_ret);
    if (!write_ret)
    {
        close_conn(); // 这里为什么选择关闭连接后，还要修改m_socket;
    }

    modfd(m_epollfd, m_sockfd, EPOLLOUT, m_TRIGMode);

    // if(read_ret==GET_REQUEST){

    // }
}

void http_conn::init(int sockfd, const sockaddr_in &addr, char *root, int TRIGMode,
                     int close_log, string user, string passwd, string sqlname)
{
    m_sockfd=sockfd;
    m_address = addr;

    addfd(m_epollfd, m_sockfd, true, m_TRIGMode);
    m_user_count++;

    doc_root=root;
    m_TRIGMode=TRIGMode;
    m_close_log=close_log;

    strcpy(sql_user,user.c_str());
    strcpy(sql_passwd,passwd.c_str());
    strcpy(sql_name,sqlname.c_str());

    init();
}


void http_conn::init()
{
    mysql = NULL;
    bytes_to_send = 0;
    bytes_have_send = 0;


    m_check_state = CHECK_STATE_REQUESTLINE;
    m_linger = false;
    m_method = GET ;
    m_url = 0;
    m_version = 0;
    m_host = 0;
    m_content_length = 0;
    m_start_line = 0;
    m_checked_idx = 0;
    m_read_idx = 0;
    m_write_idx = 0;
    m_cgi = 0;
    m_state = 0;
    timer_flag = 0;
    improv = 0;

    memset(m_read_buf, '\0', READ_BUFFER_SIZE);
    memset(m_write_buf, '\0', WRITE_BUFFER_SIZE);
    memset(m_real_file, '\0', FILENAME_LEN);
}


//循环读取客户数据，直到无数据可读或对方关闭连接
//非阻塞ET工作模式下，需要一次性将数据读完
void http_conn::close_conn(bool real_close)
{
    if(real_close && (m_sockfd != -1)){
        printf("close %d\n",m_sockfd);
        removefd(m_epollfd,m_sockfd);
        m_sockfd = -1;
        m_user_count--;
    }
}

bool http_conn::read_once()
{
    if (m_read_idx >= READ_BUFFER_SIZE)
    {
        return false;
    }

    int bytes_read = 0;

    // LT
    if (m_TRIGMode == 0)
    {
        bytes_read = recv(m_sockfd, m_read_buf + m_read_idx, READ_BUFFER_SIZE - m_read_idx, 0);
        
        m_read_idx += bytes_read;

        if (bytes_read > 0)
        {
            return true;            /* code */
        }
       
    }

    // ET 模式
    else
    {
        while (true)
        {

            bytes_read = recv(m_sockfd, m_read_buf + m_read_idx, READ_BUFFER_SIZE - m_read_idx, 0);

            if (bytes_read == -1)
            {
                if (errno == EAGAIN || errno == EWOULDBLOCK) // ewouldblock
                {
                    break;
                }
                return false;
            }
            else if (bytes_read == 0)
            {
                return false;
            }
            m_read_idx += bytes_read;
        }

        return true;
    }
    return false;
}

bool http_conn::write()
{
    int temp = 0;

    if (bytes_to_send == 0)
    {
        modfd(m_epollfd, m_sockfd, EPOLLIN, m_TRIGMode);
        init();
        return true;
    }

    while (1)
    {

        temp = writev(m_sockfd, m_iv, m_iv_count);

        if (temp < 0)
        {
            if (errno == EAGAIN)
            {
                modfd(m_epollfd, m_sockfd, EPOLLOUT, m_TRIGMode);
                return true;
            }

            unmap();
            return false;
        }

        bytes_have_send += temp;
        bytes_to_send -= temp;

        if (bytes_have_send >= m_iv[0].iov_len)
        {
            m_iv[0].iov_len = 0;
            //m_iv[1].iov_base = m_file_address + (bytes_have_send - m_iv[0].iov_len); //error
            m_iv[1].iov_base = m_file_address + (bytes_have_send - m_write_idx);

            m_iv[1].iov_len = bytes_to_send;
            /* code */
        }
        else
        {
            m_iv[0].iov_base = m_write_buf + bytes_have_send;
            m_iv[0].iov_len = m_iv[0].iov_len - bytes_have_send;
        }

        if (bytes_to_send <= 0)
        {

            unmap();

            modfd(m_epollfd, m_sockfd, EPOLLIN, m_TRIGMode);

            if (m_linger)
            {

                init();
                return true;
            }
            else
            {
                return false;
            }
        }
    }
}

void http_conn::initmysql_result(connection_pool *connpool)
{
    MYSQL * mysql = nullptr;
    connectionRAII mysqlcon(&mysql,connpool);

    if(mysql_query(mysql,"select username,passwd from user")){
        throw std::exception();
    }

    MYSQL_RES*result = mysql_store_result(mysql);

    int num_fields = mysql_num_fields(result);

    MYSQL_FIELD *fields = mysql_fetch_field(result);

    while(MYSQL_ROW row =mysql_fetch_row(result)){
        string temp1 = row[0];
        string temp2 = row[1];
        users[temp1]=temp2;
    }

}




//主状态机，每一次读取一行报文，并对报文进行解析，更新http对象中定义的相关成员，更新解析的状态；
//主状态机，在每次循环，都会调用从状态机（会返回每一行的状态，为LINE_OK，就继续解析），
//最后当解析姿态m_check_state，变为CHECK——STATE——CONTENT时，并对content解析完成后，将Line_status设为LINE——OPEN，退出主状态机循环；
//在最后中，如果提前返回了解析结果GET_REQUEST，那么是正常结束；如果通过line_status打破while循环，最终返回NO_REQUEST,也就是报文获取不完整，需要继续获取报文；
http_conn::HTTP_CODE http_conn::process_read()
{

    LINE_STATUS line_status = LINE_OK;
    HTTP_CODE ret = NO_REQUEST;
    char *text = nullptr;
    //
    while((m_check_state == CHECK_STATE_CONTENT&&line_status == LINE_OK)||(line_status=parse_line())==LINE_OK){
        //text=m_read_buf;///
        text=get_line();

        m_start_line=m_checked_idx;
    

        switch (m_check_state)
        {
        case CHECK_STATE_REQUESTLINE:
        {
            ret=parse_request_line(text);
            if(ret==BAD_REQUEST){
                return BAD_REQUEST;
            }
            break;
        }
        case CHECK_STATE_HEADER:
        {     
            ret= parse_headers(text);
            if (ret ==BAD_REQUEST)
            {
                return BAD_REQUEST;
            }
            else if(ret==GET_REQUEST){
                return do_request();
            }
            break;
        }
        case CHECK_STATE_CONTENT:
        {
            ret = parse_content(text);
            if (ret == GET_REQUEST)
            {
                return do_request();
            }

            line_status=LINE_OPEN;//break while;

            

            break;
        }
        default:
        {
            return INTERNAL_ERROR;
        }
        }


    }
    return NO_REQUEST;//zhe li ying gai jie jue l ,wo zhi qian ti chu de wen ti;




}

bool http_conn::process_write(HTTP_CODE ret)
{
    switch (ret)
    {
        case INTERNAL_ERROR:
        {
            add_status_line(500, error_500_title);
            add_headers(strlen(error_500_form));
            if (!add_content(error_500_form))
            {
                return false;
            }

            break;
        }
        case BAD_REQUEST:
        {
            add_status_line(404, error_404_title);
            add_headers(strlen(error_404_form));
            if (!add_content(error_404_form))
            {
                return false;
            }
            break;
        }
        case FORBIDDEN_REQUEST:
        {
            add_status_line(403, error_403_title);
            add_headers(strlen(error_403_form));
            if (!add_content(error_403_form))
            {
                return false;
            }
            break;
        }
        case FILE_REQUEST:
        {
            add_status_line(200, ok_200_title);
            if (m_file_stat.st_size != 0)
            {
                add_headers(m_file_stat.st_size);
                m_iv[0].iov_base = m_write_buf;
                m_iv[0].iov_len = m_write_idx;
                m_iv[1].iov_base = m_file_address;
                m_iv[1].iov_len = m_file_stat.st_size;
                m_iv_count = 2;

                bytes_to_send = m_write_idx + m_file_stat.st_size;
                return true;
            }
            else
            {
                string ok_string = "<html><body></body></html>";
                add_headers(ok_string.length()); // strlen()会不会计算\0的长度，如果计算，这里应该需要修改；
                if (!add_content(ok_string.c_str()))
                {
                    return false;
                }
            }
        }
        default:
        {
            return false;
        }
    }
    //如果没有文件需要发送，那么只需要发送m_write_buf中的内容就可以了。没有使用mmap映射的内存块;
    m_iv[0].iov_base = m_write_buf;
    m_iv[0].iov_len = m_write_idx;
    m_iv_count = 1;
    bytes_to_send = m_write_idx;
    return true;
}

//解析报文中的请求行
http_conn::HTTP_CODE http_conn::parse_request_line(char *text)
{
    char* the_text_of_space=strpbrk(text," \t");//the function is find ' ' 

    *the_text_of_space='\0';
    the_text_of_space++;

    char* theMethod=text;
    if(strcasecmp(theMethod,"GET")==0){
        m_method=GET;
    }else if(0 == strcasecmp(theMethod,"POST")){
        m_method=POST;
        m_cgi=1;
    }
    else{
        return BAD_REQUEST;
    }
    //GET  /xxx  HTTP://
    //获取url到murl，然后获取版本号version

    // the_text_of_space此时跳过了第一个空格或\t字符，但不知道之后是否还有
    // 将the_text_of_space向后偏移，通过查找，继续跳过空格和\t字符，指向请求资源的第一个字符
    // 返回字符串 the_text_of_space 中第一个不在字符串 " \t" 中出现的字符的下标。
    
    //     1   
    //GET  /xxx  HTTP://\0\0
    the_text_of_space += strspn(the_text_of_space, " \t");
    m_url=the_text_of_space;

    //          1
    //GET  /xxx  HTTP://\0\0
    the_text_of_space=strpbrk(the_text_of_space," \t");
    *the_text_of_space='\0';
    the_text_of_space++;

    //           1 
    //GET  /xxx  HTTP://\0\0
    the_text_of_space += strspn(the_text_of_space, " \t");
    m_version=the_text_of_space;

    if(strcasecmp(m_version,"HTTP/1.1")!=0){
        return BAD_REQUEST;
    }

    if (strcasecmp(m_url, "HTTP://") == 0)
    {
        m_url += 7;
        m_url = strchr(m_url, '/');
        if (!m_url)
        {
            return BAD_REQUEST; // 未找到路径分隔符
        }
    }

    if (strcasecmp(m_url, "HTTPS://")==0)
    {
        m_url += 8;
        m_url = strchr(m_url, '/');
        if (!m_url)
        {
            return BAD_REQUEST; // 未找到路径分隔符
        }
    }

    if(!m_url || m_url[0]!='/'){
        return BAD_REQUEST;
    }


    if(strlen(m_url)==1)
    {
        strcat(m_url,"judge.html");
    }

    m_check_state=CHECK_STATE_HEADER;
    return NO_REQUEST;
}

//解析报文中的请求头
http_conn::HTTP_CODE http_conn::parse_headers(char *text)
{
    if (*text == '\0')
    {

        if (m_content_length != 0)
        {
            m_check_state = CHECK_STATE_CONTENT;
            return NO_REQUEST;
        }
        return GET_REQUEST;
    }
    else if (strncasecmp(text, "Connection:", 11) == 0)
    {
        text += 11;
        text += strspn(text, " \t");
        if (strcasecmp(text, "keep-alive") == 0)
        {
            m_linger = true;
        }
        return NO_REQUEST;
    }
    else if (strncasecmp(text, "content-length:", 15) == 0)
    {
        text += 15;
        text += strspn(text, " \t");
        m_content_length = atoi(text);
        return NO_REQUEST;
    }
    else if (strncasecmp(text, "host:", 5) == 0)
    {
        text += 5;
        text += strspn(text, " \t");
        m_host = text;
        return NO_REQUEST;
    }
    else
    {

        //printf("other header: %s", text);
    }

    return NO_REQUEST;
}

//解析保文中的请求内容
http_conn::HTTP_CODE http_conn::parse_content(char *text)
{
    if(m_read_idx>=(m_content_length+m_checked_idx)){

        text[m_checked_idx+m_content_length]='\0';
        m_string=text;
        return GET_REQUEST;
    }
    return NO_REQUEST;
}

// do_request数据库，部分还没有完善；
// 在获取完报文请求后，获取到请求中的m_url，负责解析m_url是否合法，并根据url中的请求类型作出相应的转换。
// 若果url为2,3开头完成用户的注册和登陆，其他则将真正对于的请求资源路径替换到url中，
// 最后在使用【 stat() 】关联上需要访问的文件，对资源是否存在/访问权限/访问方式（访问的文件是否为文件夹）是否合法，
// 最后最后 [ mmap() ] ,将文件映射到内存中并将地址赋值给m_file_address(后续赋值给m_iv[1])，为后续的主线程进行IO，调用writev作准备；
http_conn::HTTP_CODE http_conn::do_request()
{

    /*
    webserver:
    char server_path[200];
    getcwd(server_path, 200);
    char root[] = "/root";
    m_root = (char *)malloc(strlen(server_path) + strlen(root) + 1);
    root/(html,.jpg/...)


    http_conn:
    doc_root =  m_root
    
    */

    std::string file_root = doc_root;
    // 给根目录串先插入对象的m_real_file字符串中;
    strcpy(m_real_file, file_root.c_str());

    string m_url_string(m_url);

    int root_len = file_root.length();
    // file_root+=m_url_string;

    // char* file_real_root=strcpy();;

    // char m_url_firstChar=file_root[root_len+1];//这个是根目录‘/’之后的第一个字符
    const char *p = strrchr(m_url, '/');
    char m_url_firstChar = *(p + 1);

    if (m_cgi == 1 && (m_url_firstChar == '2' || m_url_firstChar == '3'))
    {

        string real_file_temp="/";
        real_file_temp+=string(m_url+2);
        strncpy(m_real_file+root_len,real_file_temp.c_str(),FILENAME_LEN-1-root_len);

        string stringOF_m_string(m_string);
        string name, passwd;

        // 提取用户名
        size_t user_start = 5; // "user=" 的长度是 5
        size_t user_end = stringOF_m_string.find('&', user_start);
        if (user_end != std::string::npos)
        {
            name = stringOF_m_string.substr(user_start, user_end - user_start);
        }

        // 提取密码
        size_t passwd_start = user_end + 7; // "&passwd=" 的长度是 7
        if (passwd_start < stringOF_m_string.length())
        {
            passwd = stringOF_m_string.substr(passwd_start);
        }

        //....
        //user=xxx&passwd=xxx；
        if (m_url_firstChar=='3')
        {

            std::string sql_insert = "INSERT INTO user(username, passwd) VALUES('";
            sql_insert += name + "', '" + passwd + "')";


            if(users.find(name) == users.end()){

                m_lock.lock();
                int ret = mysql_query(mysql, sql_insert.c_str());

                users.insert(pair<string, string>(name, passwd));

                m_lock.unlock();

                if(!ret){
                    strcpy(m_url,"/log.html");
                }
                else{
                    strcpy(m_url,"/registerError.html");
                }
            }
            else
            {
                strcpy(m_url,"/registerError.html");
            }

            /* code */
        }
        else if(m_url_firstChar == '2')
        {
            if (users.count(name) != 0)
            {
                strcpy(m_url,"/welcome.html");
            }
            else
            {
                strcpy(m_url,"/logError.html");
            }
        }
    }

    if (m_url_firstChar == '0')
    {
        std::string real_file_temp("/register.html");
        strncpy(m_real_file + root_len, real_file_temp.c_str(), real_file_temp.length());
    }
    else if (m_url_firstChar == '1')
    {
        std::string real_file_temp("/log.html");
        strncpy(m_real_file + root_len, real_file_temp.c_str(), real_file_temp.length());
    }
    else if (m_url_firstChar == '5')
    {
        std::string real_file_temp("/picture.html");
        strncpy(m_real_file + root_len, real_file_temp.c_str(), real_file_temp.length());
    }
    else if (m_url_firstChar == '6')
    {
        std::string real_file_temp("/video.html");
        strncpy(m_real_file + root_len, real_file_temp.c_str(), real_file_temp.length());
    }
    else if (m_url_firstChar == '7')
    {
        std::string real_file_temp("/fans.html");
        strncpy(m_real_file + root_len, real_file_temp.c_str(), real_file_temp.length());
    }
    else
    {
        // 如果以上均不符合，即不是登录和注册，直接将url与网站目录拼接
        // 这里的情况是welcome界面，请求服务器上的一个图片
        strncpy(m_real_file + root_len, m_url, FILENAME_LEN - root_len - 1); //
    }


    if (stat(m_real_file,&m_file_stat)<0)
    {
        return NO_RESOURCE;
    }

    //forbidden request
    if(!(m_file_stat.st_mode&S_IROTH)){
        return FORBIDDEN_REQUEST;
    }    

    if(S_ISDIR(m_file_stat.st_mode)){
        return BAD_REQUEST;
    }

    int fd=open(m_real_file,O_RDONLY);
    
    m_file_address = (char *)mmap(0, m_file_stat.st_size, PROT_READ, MAP_PRIVATE, fd, 0);

    close(fd);
    return FILE_REQUEST;
}

//从状态机，在主状态机中被调用，负责将报文的每一行的\r\n转化为\0\0,并返回每一行的状态，也就是每一行是否完整
http_conn::LINE_STATUS http_conn::parse_line()
{
    char temp;
    for (; m_checked_idx < m_read_idx; ++m_checked_idx)
    {

        temp = m_read_buf[m_checked_idx];
        if (temp == '\r')
        {
            if(m_checked_idx+1==m_read_idx){
                return LINE_OPEN;
            }
            else if(m_read_buf[m_checked_idx+1] == '\n'){
                m_read_buf[m_checked_idx++] = '\0';
                m_read_buf[m_checked_idx++] = '\0';
                return LINE_OK;
            }
            return LINE_BAD;
        }
        else if (temp == '\n')
        {
            if (m_checked_idx > 1 && m_read_buf[m_checked_idx - 1] == '\r')
            {
                m_read_buf[m_checked_idx - 1] = '\0';
                m_read_buf[m_checked_idx++] = '\0';
                return LINE_OK;
            }
            return LINE_BAD;
        }
    }

    return LINE_OPEN;
}




//------------------//do_request

//负责真正的将响应内容写入到write_buff中，也就是被下面的三部分方法调用写入
bool http_conn::add_response(const char* format, ...)
{
    if(m_write_idx>=WRITE_BUFFER_SIZE){
        return false;
    }

    va_list arg_list;
    va_start(arg_list,format);

    int len=vsnprintf(m_write_buf+m_write_idx,WRITE_BUFFER_SIZE-1-m_write_idx,format,arg_list);

    if(len >= (WRITE_BUFFER_SIZE-1-m_write_idx)){// why it not can ==?
        va_end(arg_list);
        return false;
    }
    m_write_idx+=len;
    va_end(arg_list);

    return true;

}


//respone_line  写入响应状态
bool http_conn::add_status_line(int status,const char*title)
{

    return add_response("%s %d %s\r\n","HTTP/1.1",status,title);
}

//headers 写入报文头
bool http_conn::add_headers(int content_length)
{

    return add_content_length(content_length)&&add_linger()&&add_blank_line();
}
bool http_conn::add_content_type()
{

    return add_response("Content-Type:%s\r\n","text/html");
}
bool http_conn::add_content_length(int content_length)
{

    return add_response("Content-Length:%d\r\n",content_length);
}
bool http_conn::add_linger()
{

    return add_response("Connection:%s\r\n",m_linger==true ? "keep-alive":"close");
}
bool http_conn::add_blank_line()
{

    return add_response("%s","\r\n");
}


//content writebuff写入内容
bool http_conn::add_content(const char* content)
{
    return add_response("%s",content);

}

