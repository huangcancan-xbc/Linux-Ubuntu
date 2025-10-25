#pragma once

#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <string.h>
#include "Log.hpp"
// #include "log.cpp"

const int backlog = 10;                                     // 监听队列的最大长度

enum
{
    SocketErr = 2,                                          // 套接字创建错误
    BindErr,                                                // 绑定错误
    ListenErr,                                              // 监听错误
    NON_BLOCK_ERR,                                          // 非阻塞错误
};

class Sock
{
private:
    int sockfd_;                                            // 套接字文件描述符
public:
    Sock()
    {

    }

    ~Sock()
    {

    }

    void Socket()
    {
        sockfd_ = socket(AF_INET, SOCK_STREAM, 0);
        if(sockfd_ < 0)
        {
            log_(Fatal, "套接字创建失败: %s: %d", strerror(errno), errno);
            exit(SocketErr);
        }
        log_(Info, "套接字创建成功: fd=%d", sockfd_);
    }

    void Bind(uint16_t port)
    {
        struct sockaddr_in local;
        memset(&local, 0, sizeof(local));
        local.sin_family = AF_INET;                         // IPv4地址族
        local.sin_port = htons(port);                       // 端口号转换为网络字节序
        local.sin_addr.s_addr = INADDR_ANY;                 // 绑定到所有可用网络接口

        if(bind(sockfd_, (struct sockaddr*)&local, sizeof(local)) < 0)
        {
            log_(Fatal, "绑定失败: %s: %d", strerror(errno), errno);
            exit(BindErr);
        }
        log_(Info, "绑定成功: 端口=%d", port);
    }

    void Listen()
    {
        if(listen(sockfd_, backlog) < 0)
        {
            log_(Fatal, "监听失败: %s: %d", strerror(errno), errno);
            exit(ListenErr);
        }
        log_(Info, "监听成功: 队列长度=%d", backlog);
    }

    int Accept(string *clientip, uint16_t *clientport)
    {
        struct sockaddr_in addr;
        socklen_t len = sizeof(addr);

        int newfd = accept(sockfd_, (struct sockaddr*)&addr, &len);
        if(newfd < 0)
        {
            log_(Fatal, "接受连接失败: %s: %d", strerror(errno), errno);
            return -1;
        }

        // 提取客户端IP地址和端口号
        char ipstr[64];
        inet_ntop(AF_INET, &addr.sin_addr, ipstr, sizeof(ipstr));   // 二进制IP转字符串
        *clientip = ipstr;
        *clientport = ntohs(addr.sin_port);                // 网络字节序转主机字节序

        log_(Debug, "新连接建立: %s:%d, fd=%d", ipstr, *clientport, newfd);
        return newfd;
    }

    bool Connect(const string ip, const uint16_t &port)
    {
        struct sockaddr_in addr;
        memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;                         // IPv4地址族
        addr.sin_port = htons(port);                       // 端口号转换为网络字节序
        inet_pton(AF_INET, ip.c_str(), &(addr.sin_addr));  // 字符串IP转二进制

        // 建立连接
        int n = connect(sockfd_, (struct sockaddr*)&addr, sizeof(addr));
        if(n == -1) 
        {
            cerr << "连接失败: " << ip << ":" << port << " error" << endl;
            return false;
        }
        log_(Info, "连接成功: %s:%d", ip.c_str(), port);

        return true;
    }

    void Close()
    {
        close(sockfd_);
    }

    int Fd()
    {
        return sockfd_;
    }
};