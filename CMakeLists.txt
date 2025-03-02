cmake_minimum_required(VERSION 3.10)
project(Server)

# 设置 C++ 标准（如 C++11）
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 如果未设置构建类型，默认使用 Debug 模式
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

# 定义所有源文件列表
set(SOURCE_FILES
    main.cpp
    http/http_conn.cpp
    sql_connpool/sql_connection_pool.cpp
    timer/timer.cpp
    #webserver.cpp
    #config.cpp
    webserver/webserver.cpp
    config/config.cpp
)

# 添加头文件搜索路径（包括各子目录）
include_directories(${CMAKE_SOURCE_DIR})
include_directories(${CMAKE_SOURCE_DIR}/http)
include_directories(${CMAKE_SOURCE_DIR}/sql_connpool)
include_directories(${CMAKE_SOURCE_DIR}/timer)
include_directories(${CMAKE_SOURCE_DIR}/lock)
include_directories(${CMAKE_SOURCE_DIR}/threadpool)
include_directories(${CMAKE_SOURCE_DIR}/config)
include_directories(${CMAKE_SOURCE_DIR}/webserver)

# 添加可执行文件
add_executable(server ${SOURCE_FILES})

#将资源目录root拷贝到build目录下
file(COPY ${CMAKE_SOURCE_DIR}/root DESTINATION ${CMAKE_BINARY_DIR})


# 链接 pthread 和 mysqlclient 库
target_link_libraries(server pthread mysqlclient)
