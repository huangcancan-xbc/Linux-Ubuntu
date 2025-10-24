#pragma once

#include <iostream>
#include <sys/epoll.h>
#include "Log.hpp"
#include "noCopy.hpp"
#include <cstring>
#include <errno.h>
using namespace std;

class Epoller : public noCopy                   // 公有继承noCopy类，让Epoller对象不能被拷贝
{
    static const int size = 128;                // epoll_create的参数，告诉内核预计要监听多少个fd

public:
    Epoller()
    {
        _epfd = epoll_create(size);             // 创建epoll实例，返回epoll文件描述符
        if (_epfd == -1)
        {
            log_(Error, "epoll_create error: %s", strerror(errno)); // 创建失败，记录错误日志
        }
        else
        {
            log_(Info, "epoll_create success"); // 创建成功，记录成功日志
        }
    }

    ~Epoller()
    {
        if (_epfd >= 0)                         // 检查epfd是否有效
        {
            close(_epfd);                       // 关闭epoll文件描述符，释放资源
        }
    }

    // 更新epoll监听列表（添加、修改、删除fd）
    int EpollerUpdate(int op, int sock, uint32_t event)
    {
        int n;
        if (op == EPOLL_CTL_DEL)                // 如果是删除操作
        {
            n = epoll_ctl(_epfd, op, sock, nullptr);    // 删除时不需要event结构体，传nullptr
            if (n != 0)
            {
                log_(Error, "epoll_ctl del error: %s", strerror(errno));
            }
        }
        else                                    // 如果是添加或修改操作
        {
            struct epoll_event ev;              // 创建epoll_event结构体
            ev.events = event;                  // 设置要监听的事件类型
            ev.data.fd = sock;                  // 将socket fd保存到data中，方便后续获取

            n = epoll_ctl(_epfd, op, sock, &ev);// 添加或修改fd到epoll监听列表
            if (n != 0)
            {
                log_(Error, "epoll_ctl add error: %s", strerror(errno));
            }
        }

        return n;                               // 返回操作结果（0成功，-1失败）
    }

    int EpollerWait(struct epoll_event events[], int maxevents)
    {
        int n = epoll_wait(_epfd, events, maxevents, _timeout); // 等待事件

        return n;                               // 返回就绪事件的数量（>0），超时返回0，出错返回-1
    }

private:
    int _epfd;                                  // epoll文件描述符
    int _timeout{3000};                         // epoll_wait的超时时间，单位毫秒（默认3秒）
};