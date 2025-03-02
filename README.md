# LinuxWebServer
  This is a lightweight HTTP server for Linux


# 编译
```bash
mkdir build
cmake ..
cmake --build . / make
```

# 启动参数
```bash

-p 启动端口 (默认：9500)
-l 日志写模式 (同步/异步，默认：同步)
-m listenfd/connfd触发组合模式 ( 0 LT+LT, 1 LT+ET, 2 ET+LT, 3 ET+ET)
-o 断开连接的模式 (使用close()/shoutdown()，默认：？)
-s 数据库连接池中连接的数量 (默认：8)
-t 线程池中线程的数量 (默认：8)
-c 日志开启或关闭 (默认：0 开启， 1 关闭)
-a 并发处理模式 (Proactor/Ractor，默认：0 proactor， 1 Ractor)

```

# webbench 测试

```bash
:~/桌面/Linux_webserver/new/TinyWebServer1/test_pressur
e/webbench-1.5$ ./webbench -c 10000  -t  5   http://127.0.0.1:9500/
Webbench - Simple Web Benchmark 1.5
Copyright (c) Radim Kolar 1997-2004, GPL Open Source Software.

Benchmarking: GET http://127.0.0.1:9500/
10000 clients, running 5 sec.

Speed=1128984 pages/min, 2107436 bytes/sec.
Requests: 94082 susceed, 0 failed.

```