#include <iostream>
#include <poll.h>
#include <sys/time.h>
#include <memory>
#include "Socket.hpp"
#include "noCopy.hpp"
#include "Epoller.hpp"
using namespace std;

// 定义epoll事件类型常量
uint32_t EVENT_IN = (EPOLLIN);                              // 可读事件（EPOLLIN）
uint32_t EVENT_OUT = (EPOLLOUT);                            // 可写事件（EPOLLOUT）

class EpollServer : public noCopy                           // 继承防拷贝基类，确保服务器对象不能被拷贝
{
    static const int num = 64;                              // epoll_wait最多返回的事件数量

public:
    EpollServer(uint16_t port)                              // 构造函数，接收端口号
        : _port(port),                                      // 初始化端口号
          _listsocket_ptr(new Sock()),                      // 创建监听socket智能指针
          _epoller_ptr(new Epoller())                       // 创建epoller智能指针
    {
    }

    ~EpollServer()
    {
        _listsocket_ptr->Close();                            // 关闭监听socket
    }

    void Init()
    {
        _listsocket_ptr->Socket();                           // 创建socket
        _listsocket_ptr->Bind(_port);                        // 绑定端口
        _listsocket_ptr->Listen();                           // 开始监听

        log_(Info, "create listen socket success! fd = %d", _listsocket_ptr->Fd()); // 记录监听socket创建成功日志
    }

    // 处理新连接
    void Accept()
    {
        std::string client_ip;                               // 存储客户端IP
        uint16_t client_port;                                // 存储客户端端口
        int sockfd = _listsocket_ptr->Accept(&client_ip, &client_port);             // 接收新连接
        if (sockfd > 0)                                      // 如果接收连接成功
        {
            _epoller_ptr->EpollerUpdate(EPOLL_CTL_ADD, sockfd, EVENT_IN);           // 将新连接的fd添加到epoll监听列表，监听可读事件
            log_(Info, "获得一个新的连接：客户端说：%s:%d", client_ip.c_str(), client_port);    // 记录新连接日志
        }
    }

    // 处理数据接收
    void Recver(int fd)
    {
        char buffer[1024];                                  // 临时缓冲区，用于接收数据
        int n = read(fd, buffer, sizeof(buffer) - 1);       // 从指定fd读取数据
        if (n > 0)                                          // 成功读取到数据
        {
            buffer[n] = '\0';                               // 添加字符串结束符
            std::cout << "收到消息：" << buffer << std::endl;// 输出收到的消息

            std::string echo_str = "Sverver echo # " + std::string(buffer);     // 构造回显消息
            write(fd, echo_str.c_str(), echo_str.size());   // 将回显消息写回客户端
        }
        else if (n == 0)                                    // 客户端关闭连接（读到文件结束符）
        {
            log_(Info, "客户端断开连接: fd=%d", fd);         // 记录客户端断开连接日志
            std::cout << "客户端断开连接" << std::endl;      // 输出断开连接信息
            _epoller_ptr->EpollerUpdate(EPOLL_CTL_DEL, fd, 0);      // 从epoll监听列表中删除该fd
            close(fd);                                      // 关闭该连接的文件描述符
        }
        else                                                // 读取数据出错
        {
            log_(Error, "read error: fd=%d", fd);           // 记录读取错误日志
            std::cout << "read error" << std::endl;         // 输出错误信息
            _epoller_ptr->EpollerUpdate(EPOLL_CTL_DEL, fd, 0);      // 从epoll监听列表中删除该fd
            close(fd);                                      // 关闭该连接的文件描述符
        }
    }

    void Dispatcher(struct epoll_event revs[], int num)     // 事件分发器，处理所有就绪的事件
    {
        for (int i = 0; i < num; i++)                       // 遍历所有就绪的事件
        {
            uint32_t events = revs[i].events;               // 获取事件类型
            int fd = revs[i].data.fd;                       // 获取发生事件的文件描述符

            if (events & EVENT_IN)                          // 如果是可读事件
            {
                if (fd == _listsocket_ptr->Fd())            // 如果是监听socket可读（有新连接）
                {
                    Accept();                               // 处理新连接
                }
                else
                {
                    Recver(fd);                             // 处理数据接收
                }
            }
            else if (events & EVENT_OUT)                    // 如果是可写事件
            {
                // 可写事件处理逻辑
            }
            else                                            // 其他事件
            {
                // 其他事件处理逻辑
            }
        }
    }

    void Start()                                            // 启动服务器主循环
    {
        _epoller_ptr->EpollerUpdate(EPOLL_CTL_ADD, _listsocket_ptr->Fd(), EVENT_IN);    // 将监听socket添加到epoll监听列表
        struct epoll_event revs[num];                       // 创建epoll_event数组，用于接收就绪事件

        for (;;)                                            // 服务器无限循环
        {
            int n = _epoller_ptr->EpollerWait(revs, num);   // 等待事件发生，最多返回num个事件
            if (n > 0)                                      // 有事件发生
            {
                log_(Info, "有事件已经就绪，开始处理……其文件描述符是：%d", revs[0].data.fd);// 记录第一个就绪事件的fd
                Dispatcher(revs, n);                        // 分发处理所有就绪的事件
            }
            else if (n == 0)                                // 超时（没有事件发生）
            {
                log_(Info, "timeout...");                   // 记录超时日志
            }
            else                                            // epoll_wait出错
            {
                log_(Error, "epoll_wait error!");           // 记录错误日志
                // break;                                   // 出错时可以选择退出循环
            }
        }
    }

private:
    std::shared_ptr<Sock> _listsocket_ptr;                  // 监听socket的智能指针
    std::shared_ptr<Epoller> _epoller_ptr;                  // Epoller对象的智能指针
    uint16_t _port;                                         // 服务器端口号
};