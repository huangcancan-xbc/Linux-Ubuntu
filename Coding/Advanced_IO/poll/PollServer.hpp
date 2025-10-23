#include <iostream>
#include <poll.h>
#include <sys/time.h>
#include "Socket.hpp"
// #include "Log.hpp"
using namespace std;

static const uint16_t defaultport = 8080;           // 默认端口号8080
static const int fd_num_max = 64;                   // poll数组的最大容量，最多同时监听64个文件描述符
int defaultfd = -1;                                 // 无效文件描述符，用来标识数组中空闲的位置
int no_event = 0;

class PollServer
{
public:
    PollServer(uint16_t port = defaultport)
        : _port(port)
    {
        for (int i = 0; i < fd_num_max; i++)
        {
            _event_fds[i].fd = defaultfd;           // 初始化每个位置的fd为-1（表示未使用）
            _event_fds[i].events = no_event;        // 初始化每个位置的监听事件为0（不监听任何事件）
            _event_fds[i].revents = no_event;       // 初始化每个位置的返回事件为0（无事件发生）

            // std::cout << "fd_array[" << i << "]" << " : " << fd_array[i] << std::endl;
        }
    }

    ~PollServer()
    {
        _listensock.Close();                        // 关闭监听socket
    }

    // 初始化服务器：创建socket、绑定端口、开始监听
    bool Init()
    {
        _listensock.Socket();                       // 创建监听socket
        _listensock.Bind(_port);                    // 绑定端口
        _listensock.Listen();                       // 开始监听

        return true;
    }

    // 打印当前在线的文件描述符列表
    void PrintFd()
    {
        cout << "在线_fd_列表： ";                  // 输出提示信息
        for (int i = 0; i < fd_num_max; i++)       // 遍历_event_fds数组
        {
            if (_event_fds[i].fd == defaultfd)     // 如果是空闲位置（fd为-1），跳过
            {
                continue;
            }

            cout << _event_fds[i].fd << " ";       // 输出有效的文件描述符
        }

        cout << endl;
    }

    void Accepter()
    {
        // 有新的客户端连接请求到来
        std::string client_ip;                     // 客户端的IP
        uint16_t client_port;                      // 客户端的端口

        int sock = _listensock.Accept(&client_ip, &client_port);    // 接收新的客户端连接
        if (sock < 0)
        {
            return;                                // 如果accept失败，直接返回
        }

        log_(Info, "新连接建立成功：客户端IP：%s，客户端端口：%d，socket fd：%d", client_ip.c_str(), client_port, sock);
        cout << "Info, 新连接建立成功：客户端IP：" << client_ip << "，客户端端口：" << client_port << "，socket fd：" << sock << endl;

        // 将新连接的文件描述符添加到poll数组中
        int pos = 1;                               // 从数组索引1开始找空闲位置（索引0存放监听socket）
        for (; pos < fd_num_max; pos++)            // 查找数组中空闲的位置
        {
            if (_event_fds[pos].fd != defaultfd)   // 如果当前位置已被占用（fd不为-1），继续找下一个
            {
                continue;
            }
            else                                   // 找到空闲位置，跳出循环
            {
                break;
            }
        }

        if (pos == fd_num_max)                     // 如果遍历完整个数组都没找到空闲位置
        {
            log_(Warning, "服务器已满，立即关闭%d！", sock);        // 服务器连接数已达上限，关闭新连接
            cout << "Warning, 服务器已满，立即关闭" << sock << "！" << endl;
            close(sock);
        }
        else // 找到了空闲位置
        {
            _event_fds[pos].fd = sock;             // 将新连接的fd放入数组
            _event_fds[pos].events = POLLIN;       // 设置该fd的监听事件为POLLIN（监听数据可读）
            _event_fds[pos].revents = no_event;    // 清空返回事件

            PrintFd();                             // 打印当前在线的fd列表

            // 后续可能还有其他处理
        }
    }

    // 处理数据接收的函数
    void Recver(int fd, int pos)                   // fd是需要读取数据的文件描述符，pos是该fd在数组中的位置
    {
        // 接收客户端发送的数据
        char buffer[1024];                         // 临时缓冲区，用于存储接收的数据
        ssize_t n = read(fd, buffer, sizeof(buffer) - 1);       // 从指定fd读取数据到缓冲区，预留1字节给字符串结束符
        if (n > 0)                                 // 读取到数据
        {
            buffer[n] = '\0';                      // 手动添加字符串结束符
            cout << "收到一条消息：" << buffer << endl;          // 输出接收到的消息
        }
        else if (n == 0)                           // 客户端断开连接（读到文件结束符）
        {
            log_(Info, "客户端退出了，关闭连接的文件描述符是%d", fd);   // 记录客户端断开日志
            close(fd);                             // 关闭该连接的文件描述符
            _event_fds[pos].fd = defaultfd;        // 将数组中对应位置重置为-1，表示该位置空闲（从poll监听列表中移除该fd）
        }
        else                                       // 读取错误
        {
            log_(Warning, "接收文件错误，描述符是: %d", fd);      // 记录接收错误日志
            close(fd);                             // 关闭该连接的文件描述符
            _event_fds[pos].fd = defaultfd;        // 将数组中对应位置重置为-1，表示该位置空闲（从poll监听列表中移除该fd）
        }
    }

    // 事件分发函数：处理所有就绪的事件
    void Dispatcher()                              // 遍历poll数组，检查哪些fd的事件已经就绪
    {
        for (int i = 0; i < fd_num_max; i++)
        {
            int fd = _event_fds[i].fd;             // 获取数组中第i个位置的fd
            if (fd == defaultfd)                   // 如果是无效fd（-1），跳过
            {
                continue;
            }

            if (_event_fds[i].revents & POLLIN)    // 如果这个fd的POLLIN事件已经就绪（有数据可读）
            {
                if (fd == _listensock.Fd())        // 如果就绪的fd是监听socket
                {
                    Accepter();                    // 说明有新的连接请求，调用Accepter处理
                }
                else                               // 如果就绪的fd不是监听socket，而是普通的数据连接socket
                {
                    Recver(fd, i);                 // 调用Recver处理数据接收
                }
            }
        }
    }

    // 服务器主循环：使用poll实现IO多路复用
    void Start()
    {
        _event_fds[0].fd = _listensock.Fd();       // 把监听socket的fd放到数组的第一个位置
        _event_fds[0].events = POLLIN;             // 监听socket只关心POLLIN事件（新连接请求）
        int timeout = 3000;                        // 设置poll超时时间为3000毫秒（3秒），如果3秒内没有事件发生，poll会返回0

        for (;;)                                   // 服务器无限循环，持续监听和处理事件
        {
            int n = poll(_event_fds, fd_num_max, timeout);   // 调用poll等待事件发生

            // 根据poll返回值进行处理
            switch (n)
            {
            // poll超时处理（timeout时间内没有事件发生）
            case 0:
                cout << "time out, timeout: " << timeout << endl;   // poll超时，没有事件发生
                break;
            // poll调用出错处理
            case -1:
                cout << "select error, errno:" << errno << endl;    // poll调用出错
                break;
            // poll成功处理（有事件发生）
            default:
                cout << "得到了一个新的连接请求！" << endl;           // poll成功，有事件发生（注意：这里注释可能不准确，有事件发生不一定都是连接请求）
                Dispatcher();                      // 调用Dispatcher处理所有就绪的事件
                break;
            }
        }
    }

private:
    Sock _listensock;                             // 监听socket对象
    uint16_t _port;                               // 服务器端口号
    struct pollfd _event_fds[fd_num_max];         // poll需要的数组，存储要监听的fd和事件（结构体数组，数组每一个位置都是结构体）
    // struct pollfd *_event_fds;

    // int fd_array[fd_num_max];
    // int wfd_array[fd_num_max];                 // 扩展写事件处理
};