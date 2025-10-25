#pragma once

#include <iostream>
#include <string>
#include <memory>
#include <cerrno>
#include <functional>
#include <unordered_map>
#include "Log.hpp"
#include "noCopy.hpp"
#include "Epoller.hpp"
#include "Socket.hpp"
#include "Comm.hpp"
#include <cstring>

// 前向声明Connection和TcpServer类，避免循环包含
class Connection;
class TcpServer;

uint32_t EVENT_IN = (EPOLLIN | EPOLLET);            // 定义读事件掩码（EPOLLIN | EPOLLET）
uint32_t EVENT_OUT = (EPOLLOUT | EPOLLET);          // 定义写事件掩码（EPOLLOUT | EPOLLET）
const static int g_buffer_size = 1024;              // 定义缓冲区大小为1024字节

// using func_t = std::function<void(std::shared_ptr<Connection>)>;
using func_t = std::function<void(std::weak_ptr<Connection>)>;      // 定义回调函数类型，参数为weak_ptr类型的连接对象
using except_func = std::function<void(std::weak_ptr<Connection>)>; // 定义异常处理函数类型

// Connection类：管理单个客户端连接
class Connection
{
public:
    // 构造函数，接收socket文件描述符
    // Connection(int sock, std::shared_ptr<TcpServer> tcp_server_ptr)
    //     : _sock(sock),
    //     _tcp_server_ptr(tcp_server_ptr)
    // {

    // }
    Connection(int sock)
        : _sock(sock)
    {

    }

    // 设置连接的回调函数
    // void SetHandler(func_t recv_cb, func_t send_cb, func_t except_cb)
    // {
    //     _recv_cb = recv_cb;
    //     _send_cb = send_cb;
    //     _except_cb = except_cb;
    // }
    void SetHandler(func_t recv_cb, func_t send_cb, except_func except_cb)
    {
        _recv_cb = recv_cb;
        _send_cb = send_cb;
        _except_cb = except_cb;
    }

    // 获取socket文件描述符
    int SockFd()
    {
        return _sock;
    }

    // 向输入缓冲区追加数据
    void AppendInBuffer(const std::string& data)
    {
        _inbuffer += data;
    }

    // 向输出缓冲区追加数据
    void AppendOutBuffer(const std::string& info)
    {
        _outbuffer += info;
    }

    // const std::string& Inbuffer()
    // {
    //     return _inbuffer;
    // }
    // 获取输入缓冲区的引用
    std::string& Inbuffer()
    {
        return _inbuffer;
    }

    // 获取输出缓冲区的引用
    std::string& OutBuffer()
    {
        return _outbuffer;
    }

    // 设置指向TCP服务器的弱引用指针
    void SetWeakPtr(std::weak_ptr<TcpServer> tcp_server_ptr)
    {
        _tcp_server_ptr = tcp_server_ptr;
    }

    ~Connection()
    {

    }

private:
    int _sock;                      // socket文件描述符
    std::string _inbuffer;          // 输入缓冲区
    std::string _outbuffer;         // 输出缓冲区
public:
    func_t _recv_cb;                // 读回调函数
    func_t _send_cb;                // 写回调函数
    // func_t _except_cb;
    except_func _except_cb;         // 异常回调函数

    std::weak_ptr<TcpServer> _tcp_server_ptr;   // 指向TCP服务器的弱引用
    std::string _ip;                // 客户端IP地址
    uint16_t _port;                 // 客户端端口号
};



// TcpServer类：TCP服务器主类，支持epoll事件驱动
class TcpServer : public std::enable_shared_from_this<TcpServer>, public noCopy
{
    static const int num = 64;      // 定义epoll事件数组大小为64

public:
    TcpServer(uint16_t port, func_t OnMessage)          // 构造函数，接收端口号和消息处理函数
        : _port(port),                                  // 服务器端口号
        _OnMessage(OnMessage),                          // 消息处理函数
        _quit(true),                                    // 退出标志
        _epoller_ptr(new Epoller()),                    // epoll对象指针
        _listensock_ptr(new Sock())                     // 监听socket对象指针
    {

    }

    void Init()                     // 初始化服务器
    {
        _listensock_ptr->Socket();  // 创建socket
        SetNonBlockOrDie(_listensock_ptr->Fd());        // 设置为非阻塞模式
        _listensock_ptr->Bind(_port);   // 绑定端口
        _listensock_ptr->Listen();  // 开始监听

        // log_(Info, "创建listen socket成功，fd：", _listensock_ptr->Fd());
        log_(Info, "TCP服务器初始化成功，监听端口: %d, listen socket fd: %d", _port, _listensock_ptr->Fd());

        // 将监听socket添加到epoll中，设置读事件回调
        AddConnection(_listensock_ptr->Fd(), EVENT_IN, std::bind(&TcpServer::Accepter, this, std::placeholders::_1), nullptr, nullptr);
    }

    // void AddConnection(int sockfd, uint32_t event, func_t recv_cb, func_t send_cb, func_t except_cb, const std::string &ip = "0.0.0.0", uint16_t port = 0)
    // {
    //     // std::shared_ptr<Connection> new_connection=std::make_shared<Connection>(sockfd, std::shared_ptr<TcpServer>(this));
    //     std::shared_ptr<Connection> new_connection = std::make_shared<Connection>(sockfd, std::shared_ptr<TcpServer>(this));
    //     new_connection->SetHandler(recv_cb, send_cb, except_cb);
    //     new_connection->_ip=ip;
    //     new_connection->_port=port;

    //     _connections.insert(std::make_pair(sockfd, new_connection));

    //     _epoller_ptr->EpollerUpdate(EPOLL_CTL_ADD, sockfd, event);

    //     log_(Debug, "添加一个新的连接，fd：", sockfd);
    // }
    // 添加连接到服务器
    void AddConnection(int sockfd, uint32_t event, func_t recv_cb, func_t send_cb, except_func except_cb, const std::string& ip = "0.0.0.0", uint16_t port = 0)
    {
        std::shared_ptr<Connection> new_connection(new Connection(sockfd));     // 创建新的连接对象
        new_connection->SetWeakPtr(shared_from_this());                         // 设置连接对象对服务器的弱引用
        new_connection->SetHandler(recv_cb, send_cb, except_cb);                // 设置连接的回调函数

        // 设置客户端IP和端口
        new_connection->_ip = ip;
        new_connection->_port = port;

        _connections.insert(std::make_pair(sockfd, new_connection));            // 将连接添加到连接映射表中

        _epoller_ptr->EpollerUpdate(EPOLL_CTL_ADD, sockfd, event);              // 将socket添加到epoll监控列表中

        log_(Debug, "成功添加新连接，fd: %d, 客户端IP: %s, 客户端端口: %d", sockfd, ip.c_str(), port);
    }

    // 链接管理器
    // void Accepter(std::shared_ptr<Connection> connection)
    // {
    //     while (true)
    //     {
    //         struct sockaddr_in peer;
    //         socklen_t len = sizeof(peer);

    //         int sockfd = ::accept(connection->SockFd(), (struct sockaddr*)&peer, &len);
    //         if (sockfd > 0)
    //         {
    //             uint16_t peer_port = ntohs(peer.sin_port);
    //             char ipbuf[128];
    //             inet_ntop(AF_INET, &peer.sin_addr.s_addr, ipbuf, sizeof(ipbuf));

    //             log_(Debug, "收到一个新的客户端连接，得到的消息：[%s:%d], sockfd：%d", ipbuf, peer_port, sockfd);

    //             SetNonBlockOrDie(sockfd);
    //             // listensock只需要设置_recv_cb，其他sock，读，写，异常都设置
    //             // AddConnection(sockfd, EVENT_IN, nullptr, nullptr, nullptr);
    //             AddConnection(sockfd, EVENT_IN, std::bind(&TcpServer::Recver, this, std::placeholders::_1), std::bind(&TcpServer::Sender, this, std::placeholders::_1), std::bind(&TcpServer::Excepter, this, std::placeholders::_1), ipbuf, peer_port);
    //         }
    //         else
    //         {
    //             if (errno == EWOULDBLOCK)
    //             {
    //                 break;
    //             }
    //             else if (errno == EINTR)
    //             {
    //                 continue;
    //             }
    //             else
    //             {
    //                 break;
    //             }
    //         }
    //     }
    // }

    // 接受新连接的处理函数
    void Accepter(std::weak_ptr<Connection> connection)
    {
        auto connection_ptr = connection.lock();                // 将weak_ptr转换为shared_ptr以安全访问连接对象
        if (!connection_ptr)                                    // 检查转换是否成功
        {
            // log_(Warning, "无法获取连接对象指针，可能已被销毁");
            return;
        }
        
        while (true)
        {
            struct sockaddr_in peer;
            socklen_t len = sizeof(peer);

            int sockfd = ::accept(connection_ptr->SockFd(), (struct sockaddr*)&peer, &len);         // 接受新的客户端连接
            if (sockfd > 0)
            {
                uint16_t peer_port = ntohs(peer.sin_port);
                char ipbuf[128];
                inet_ntop(AF_INET, &peer.sin_addr.s_addr, ipbuf, sizeof(ipbuf));

                // log_(Debug, "收到一个新的客户端连接，得到的消息：[%s:%d], sockfd：%d", ipbuf, peer_port, sockfd);
                log_(Info, "收到新的客户端连接，客户端IP: %s, 客户端端口: %d, 连接fd: %d", ipbuf, peer_port, sockfd);

                SetNonBlockOrDie(sockfd);       // 设置新连接为非阻塞模式

                // 为新连接添加到服务器管理中，设置各种回调函数
                AddConnection(sockfd, EVENT_IN,
                    std::bind(&TcpServer::Recver, this, std::placeholders::_1),
                    std::bind(&TcpServer::Sender, this, std::placeholders::_1),
                    std::bind(&TcpServer::Excepter, this, std::placeholders::_1),
                    ipbuf, peer_port);
            }
            else
            {
                if (errno == EWOULDBLOCK)       // 处理accept返回错误的情况
                {
                    break;                      // 没有更多连接，退出循环
                }
                else if (errno == EINTR)
                {
                    continue;                   // 被信号中断，继续循环
                }
                else
                {
                    log_(Error, "accept调用失败，错误码: %d, 错误信息: %s", errno, strerror(errno));
                    break;                      // 其他错误，退出循环
                }
            }
        }
    }

    // 事件管理器
    // void Recver(std::shared_ptr<Connection> connection)
    // {
    //     // std::cout << "haha, got you!!!, sockfd:" << connection->SockFd() << std::endl;
    //     int sockfd = connection->SockFd();

    //     while(true)
    //     {
    //         char buffer[g_buffer_size];
    //         memset(buffer, 0, sizeof(buffer));

    //         ssize_t n = recv(sockfd, buf, sizeof(buffer) - 1, 0);
    //         if (n > 0)
    //         {
    //             connection->Append(buffer);
    //         }
    //         else if(n == 0)
    //         {
    //             log_(Debug,"客户端断开连接，" ,sockfd,  connection->_ip.c_str(),connection->_port);
    //             connection->excepter(connection);
    //         }
    //         else
    //         {
    //             if (errno ==EWOULDBLOCK)
    //             {
    //                 break;
    //             }
    //             else if (errno == EINTR)
    //             {
    //                 continue;
    //             }
    //             else
    //             {
    //                 log_(Warning,"sockfd: %d,客户端断开连接，errno: %d",fd,connection->_ip.c_str(),connection->_port);
    //                 connection->excepter(connection);
    //                 break;
    //             }
    //         }
    //     }

    //     OnMessage(connection);
    // }

    // 接收数据的处理函数
    void Recver(std::weak_ptr<Connection> connection)
    {
        if (connection.expired())           // 检查连接对象是否已经过期
        {
            // log_(Warning, "连接对象已过期，无法处理接收事件");
            return;
        }

        // 将weak_ptr转换为shared_ptr以安全访问连接对象
        auto connection_ptr = connection.lock();
        int sockfd = connection_ptr->SockFd();

        while (true)
        {
            char buffer[g_buffer_size];
            memset(buffer, 0, sizeof(buffer));

            ssize_t n = recv(sockfd, buffer, sizeof(buffer) - 1, 0);    // 从socket接收数据
            if (n > 0)
            {
                connection_ptr->AppendInBuffer(buffer);         // 接收到数据，添加到输入缓冲区
                log_(Debug, "从客户端fd: %d 接收到 %ld 字节数据", sockfd, n);
            }
            else if (n == 0)                                    // 客户端关闭连接
            {
                // log_(Debug, "sockfd: %d,客户端消息：%s : %d断开连接(退出)", sockfd, connection_ptr->_ip.c_str(), connection_ptr->_port);
                log_(Info, "客户端fd: %d (IP: %s, 端口: %d) 主动断开连接", sockfd, connection_ptr->_ip.c_str(), connection_ptr->_port);
                connection_ptr->_except_cb(connection_ptr);     // 调用异常回调函数处理连接断开
                return;
            }
            else
            {
                if (errno == EWOULDBLOCK)                       // 处理接收错误
                {
                    // log_(Debug, "客户端fd: %d 数据接收完毕", sockfd);
                    break;                                      // 没有更多数据可读，退出循环
                }
                else if (errno == EINTR)                        // 被信号中断，继续循环
                {
                    continue;
                }
                else                                            // 其他错误
                {
                    // log_(Warning, "sockfd: %d,客户端信息：%s : %d接收错误", sockfd, connection_ptr->_ip.c_str(), connection_ptr->_port);
                    log_(Error, "从客户端fd: %d 接收数据失败，错误码: %d, 错误信息: %s", sockfd, errno, strerror(errno));
                    connection_ptr->_except_cb(connection_ptr);
                    return;
                }
            }
        }

        _OnMessage(connection_ptr);                             // 调用上层消息处理函数处理接收到的数据
    }

    // 发送数据的处理函数
    void Sender(std::weak_ptr<Connection> connection)
    {
        if (connection.expired())
        {
            // log_(Warning, "连接对象已过期，无法处理发送事件");
            return;             // 检查连接对象是否已经过期
        }

        // 将weak_ptr转换为shared_ptr以安全访问连接对象
        auto connection_ptr = connection.lock();
        auto& outbuffer = connection_ptr->OutBuffer();
        while (true)
        {
            // 发送数据到客户端
            ssize_t n = send(connection_ptr->SockFd(), outbuffer.c_str(), outbuffer.size(), 0);
            if (n > 0)
            {
                // log_(Debug, "向客户端fd: %d 成功发送 %ld 字节数据", connection_ptr->SockFd(), n);

                // 成功发送部分数据，从输出缓冲区中删除已发送的数据
                outbuffer.erase(0, n);
                if (outbuffer.empty())
                {
                    // log_(Debug, "客户端fd: %d 输出缓冲区已清空", connection_ptr->SockFd());
                    break;                  // 缓冲区清空，退出循环
                }
            }
            else if (n == 0)
            {
                // log_(Debug, "向客户端fd: %d 发送0字节数据", connection_ptr->SockFd());
                return;                     // 发送0字节，退出
            }
            else
            {
                // 处理发送错误
                if (errno == EWOULDBLOCK)
                {
                    log_(Debug, "客户端fd: %d socket发送缓冲区已满，等待下次发送", connection_ptr->SockFd());
                    break;                  // socket缓冲区满，退出循环
                }
                else if (errno == EINTR)
                {
                    continue;               // 被信号中断，继续循环
                }
                else                        // 其他错误
                {
                    // log_(Warning, "sockfd: %d, 客户端消息： %s : %d 发送错误", connection_ptr->SockFd(), connection_ptr->_ip.c_str(), connection_ptr->_port);
                    log_(Error, "向客户端fd: %d 发送数据失败，错误码: %d, 错误信息: %s", connection_ptr->SockFd(), errno, strerror(errno));
                    connection_ptr->_except_cb(connection_ptr);
                    return;
                }
            }
        }
        if (!outbuffer.empty())
        {
            // 开启对写事件的关心
            EnableEvent(connection_ptr->SockFd(), true, true);
            // log_(Debug, "为客户端fd: %d 启用写事件监控", connection_ptr->SockFd());
        }
        else
        {
            // 关闭对写事件的关心
            EnableEvent(connection_ptr->SockFd(), true, false);
            // log_(Debug, "为客户端fd: %d 禁用写事件监控", connection_ptr->SockFd());
        }

    }

    // 异常处理函数
    void Excepter(std::weak_ptr<Connection> connection)
    {
        if (connection.expired())
        {
            // log_(Warning, "连接对象已过期，无法处理异常事件");
            return;             // 检查连接对象是否已经过期
        }

        auto conn = connection.lock();          // 将weak_ptr转换为shared_ptr以安全访问连接对象
        if (!conn)                              // 检查转换是否成功
        {
            // log_(Warning, "无法获取连接对象指针，可能已被销毁");
            return;
        }

        int fd = conn->SockFd();
        // log_(Warning, "异常处理程序套接字文件描述符: %d, 客户端消息：%s : %d 异常退出", conn->SockFd(), conn->_ip.c_str(), conn->_port);
        log_(Warning, "处理异常连接，fd: %d, 客户端IP: %s, 客户端端口: %d", conn->SockFd(), conn->_ip.c_str(), conn->_port);
        
        // 1. 移除对特定fd的关心
        // EnableEvent(connection->SockFd(), false, false);
        _epoller_ptr->EpollerUpdate(EPOLL_CTL_DEL, fd, 0);
        // 2. 关闭异常的文件描述符
        log_(Debug, "关闭异常连接的文件描述符: %d", fd);
        close(fd);
        // 3. 从unordered_map中(连接映射表中)移除
        log_(Debug, "从连接管理器中移除异常连接: %d", fd);

        _connections.erase(fd);
        log_(Info, "异常连接处理完成，fd: %d", fd);
    }

    // 启用或禁用socket的读写事件监控
    void EnableEvent(int sock, bool readable, bool writeable)
    {
        uint32_t events = 0;

        // 根据参数设置事件掩码
        events |= ((readable ? EPOLLIN : 0) | (writeable ? EPOLLOUT : 0) | EPOLLET);
        _epoller_ptr->EpollerUpdate(EPOLL_CTL_MOD, sock, events);
        // log_(Debug, "更新socket: %d 的事件监控，可读: %s, 可写: %s", sock, readable ? "是" : "否", writeable ? "是" : "否");
    }

    // 检查指定的socket文件描述符是否在服务器管理中
    bool IsConnectionSafe(int sockfd)
    {
        auto iter = _connections.find(sockfd);
        if (iter == _connections.end())
        {
            // log_(Debug, "检查连接安全状态，fd: %d 不存在于连接管理器中", sockfd);
            return false;
        }
        else
        {
            // log_(Debug, "检查连接安全状态，fd: %d 存在于连接管理器中", sockfd);
            return true;
        }
    }

    // 事件分发器，处理epoll返回的事件
    void Dispatcher(int timeout)
    {
        int n = _epoller_ptr->EpollerWait(revs, num, timeout);  // 等待epoll事件

        // if (n < 0)
        // {
        //     log_(Error, "epoll_wait调用失败，错误码: %d, 错误信息: %s", errno, strerror(errno));
        //     return;
        // }
        // else if (n == 0)
        // {
        //     log_(Debug, "epoll_wait超时，未检测到任何事件");
        //     return;
        // }

        // log_(Debug, "epoll_wait返回 %d 个事件", n);


        for (int i = 0; i < n; i++)
        {
            uint32_t events = revs[i].events;
            int sockfd = revs[i].data.fd;

            // if (evets & EPOLLERR)
            // {
            //     events |= (EPOLLIN | EPOLLOUT);
            // }

            // if (evets & EPOLLHUP)
            // {
            //     events |= (EPOLLIN | EPOLLOUT);
            // }

            // 处理读事件
            if ((events & EPOLLIN) && IsConnectionSafe(sockfd))
            {
                if (_connections[sockfd]->_recv_cb)
                {
                    // log_(Debug, "处理读事件，fd: %d", sockfd);
                    _connections[sockfd]->_recv_cb(_connections[sockfd]);   // 调用读回调函数
                }
            }

            // 处理写事件
            if ((events & EPOLLOUT) && IsConnectionSafe(sockfd))
            {
                if (_connections[sockfd]->_send_cb)
                {
                    // log_(Debug, "处理写事件，fd: %d", sockfd);
                    _connections[sockfd]->_send_cb(_connections[sockfd]);   // 调用写回调函数
                }
            }
        }
    }

    // 服务器主循环
    void Loop()
    {
        _quit = false;
        log_(Info, "TCP服务器主循环开始运行，端口: %d", _port);

        // AddConnection(_listensock_ptr->Fd(), EVENT_IN, std::bind(&TcpServer::Accept, this, std::placeholders::_1),nullptr,nullptr);

        while (!_quit)              // 持续处理事件直到服务器退出
        {
            Dispatcher(3000);
            PrintConnection();      // 可选，如果需要打印连接状态可以取消注释
        }

        log_(Info, "TCP服务器主循环结束");
        _quit = true;
    }

    // 打印当前连接列表（调试用）
    void PrintConnection()
    {
        log_(Debug, "当前连接总数: %zu", _connections.size());
        std::cout << "当前连接列表:";
        for (auto& connection : _connections)
        {
            std::cout << connection.second->SockFd() << ", ";
            std::cout << "inbuffer: " << connection.second->Inbuffer().c_str();
        }
        std::cout << std::endl;
    }

    ~TcpServer()
    {
        log_(Info, "TCP服务器正在关闭，清理资源");
    }

private:
    std::shared_ptr<Epoller> _epoller_ptr;      // epoll对象指针
    std::shared_ptr<Sock> _listensock_ptr;      // 监听socket对象指针
    std::unordered_map<int, std::shared_ptr<Connection>> _connections;  // 连接管理映射表
    struct epoll_event revs[num];               // epoll事件数组
    uint16_t _port;                             // 服务器端口号
    bool _quit;                                 // 退出标志

    func_t _OnMessage;                          // 消息处理函数
};