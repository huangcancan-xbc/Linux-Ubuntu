#include <iostream>
#include <sys/select.h>
#include <sys/time.h>
#include "Socket.hpp"
// #include "Log.hpp"
using namespace std;

static const uint16_t defaultport = 8080;                   // 默认端口号8080
static const int fd_num_max = (sizeof(fd_set) * 8);         // 计算fd_set能容纳的最大文件描述符数量，每个bit位代表一个fd
int defaultfd = -1;                                         // 无效文件描述符，用来标识数组中空闲的位置

class SelectServer
{
public:
    SelectServer(uint16_t port = defaultport)
        : _port(port)
    {
        for(int i = 0; i < fd_num_max; i++)
        {
            fd_array[i] = defaultfd;                        // 将数组中所有位置都初始化为-1，表示空闲
            // std::cout << "fd_array[" << i << "]" << " : " << fd_array[i] << std::endl;
        }
    }

    ~SelectServer()
    {
        _listensock.Close();                                // 关闭监听socket
    }

    // 初始化服务器：创建socket、绑定端口、开始监听
    bool Init()
    {
        _listensock.Socket();                               // 创建监听socket
        _listensock.Bind(_port);                            // 绑定端口
        _listensock.Listen();                               // 开始监听

        return true;
    }

    // 打印当前在线的文件描述符列表
    void PrintFd()
    {
        cout << "在线_fd_列表： ";                          // 输出提示信息
        for (int i = 0; i < fd_num_max; i++)               // 遍历fd_array数组
        {
            if (fd_array[i] == defaultfd)                  // 如果是空闲位置，跳过
            {
                continue;
            }
            
            cout << fd_array[i] << " ";                    // 输出有效的文件描述符
        }

        cout << endl;
    }

    void Accepter()
    {
        // 连接事件已经就绪
        std::string client_ip;                              // 客户端的IP
        uint16_t client_port;                               // 客户端的端口

        int sock = _listensock.Accept(&client_ip, &client_port);    // 从监听socket获取新的连接，不会阻塞，因为select已经告诉我们有连接事件就绪
        if (sock < 0)
        {
            return;  // 如果accept失败，直接返回
        }

        log_(Info, "新连接建立成功：客户端IP：%s，客户端端口：%d，socket fd：%d", client_ip.c_str(), client_port, sock);
        cout << "Info, 新连接建立成功：客户端IP：" << client_ip << "，客户端端口：" << client_port << "，socket fd：" << sock << endl;

        // 将新连接的文件描述符存入fd_array数组中
        int pos = 1;                                       // 从数组索引1开始找空闲位置（索引0存放监听socket）
        for (; pos < fd_num_max; pos++)                    // 查找数组中空闲的位置
        {
            if (fd_array[pos] != defaultfd)                // 如果当前位置不为空（-1），继续找下一个
            {
                continue;
            }
            else                                           // 找到空闲位置，跳出循环
            {
                break;
            }
        }

        if (pos == fd_num_max)                             // 如果遍历完整个数组都没找到空闲位置
        {
            log_(Warning, "服务器已满，立即关闭%d！", sock); // 服务器已满，关闭新连接
            cout << "Warning, 服务器已满，立即关闭" << sock << "！" << endl;
            close(sock);
        }
        else                                               // 找到了空闲位置
        {
            fd_array[pos] = sock;                          // 将新连接的fd放入数组
            PrintFd();                                     // 打印当前在线的fd列表
            
            // 后续可能还有其他处理
        }
    }

    // 处理数据接收的函数
    void Recver(int fd, int pos)                            // fd是需要读取数据的文件描述符，pos是该fd在数组中的位置
    {
        // demo，接收客户端数据
        char buffer[1024];                                  // 临时缓冲区，用于存储接收的数据
        ssize_t n = read(fd, buffer, sizeof(buffer) - 1);   // 从指定fd读取数据到缓冲区，预留1字节给字符串结束符
        if (n > 0)                                          // 读取到数据
        {
            buffer[n] = 0;                                  // 手动添加字符串结束符
            cout << "收到一条消息：" << buffer << endl;      // 输出接收到的消息
        }
        else if (n == 0)                                    // 客户端断开连接（读到文件结束符）
        {
            log_(Info, "客户端退出了，关闭连接的文件描述符是%d", fd);  // 记录客户端断开日志
            close(fd);                                      // 关闭该连接的文件描述符
            fd_array[pos] = defaultfd;                      // 将数组中对应位置重置为-1，表示该位置空闲（这里本质是从select中移除该fd）
        }
        else                                                // 读取错误
        {
            log_(Warning, "接收文件错误，描述符是: %d", fd);  // 记录接收错误日志
            close(fd);                                      // 关闭该连接的文件描述符
            fd_array[pos] = defaultfd;                      // 将数组中对应位置重置为-1，表示该位置空闲（这里本质是从select中移除该fd）
        }
    }

    // 事件分发函数：根据select返回的结果，处理就绪的文件描述符
    void Dispatcher(fd_set& rfds)                           // rfds是select调用后被修改的fd_set，其中包含了就绪的fd
    {
        for (int i = 0; i < fd_num_max; i++)
        {
            int fd = fd_array[i];                           // 获取数组中第i个位置的fd
            if (fd == defaultfd)                            // 如果是无效fd（-1），跳过
            {
                continue;
            }
            
            // 检查这个fd是否在select返回的就绪fd集合中
            if(FD_ISSET(fd, &rfds))                         // FD_ISSET宏用于检查fd是否在rfds集合中
            {
                if(fd == _listensock.Fd())                  // 如果就绪的fd是监听socket
                {
                    Accepter();                             // 说明有新的连接请求，调用Accepter处理
                }
                else                                        // 如果就绪的fd不是监听socket，而是普通的数据连接socket
                {
                    Recver(fd, i);                          // 调用Recver处理数据接收
                }
            }
        }
    }

    // 服务器主循环：使用select实现IO多路复用
    void Start()
    {
        int listensock = _listensock.Fd();                  // 获取监听socket的文件描述符
        fd_array[0] = listensock;                           // 将监听socket放在数组第0个位置
        // 程序启动时，只有数组第0个位置有值，是监听socket的fd，但是，程序运行过程中，Accepter 会向数组中其他位置添加新的连接fd！

        for (;;)                                            // 无限循环，服务器持续运行
        {
            fd_set readfds;                                 // 定义一个fd_set变量，用于存储待检测的读就绪文件描述符集合
            FD_ZERO(&readfds);                              // 清空readfds集合，将所有位都设置为0
            // 每次进入循环，都要重新构建readfds集合，因为select会修改它。

            int maxfds = fd_array[0];                       // 记录当前最大的文件描述符值，用于select调用
            for (size_t i = 0; i < fd_num_max;i++)          // 第一次循环，遍历fd_array数组，准备select的参数
            {
                if (fd_array[i] == defaultfd)               // 如果数组中该位置是空闲的（-1），跳过
                {
                    continue;
                    // 这个 continue 非常有用！如果不跳过，fd_array[i] 是 -1，FD_SET(-1, &readfds) 会出错或行为未定义。
                    // 并且，如果当前只有监听socket，数组后面的位置都是-1，这个循环会遍历所有位置，
                    // 但由于 continue，只有有效的fd（比如监听socket）会被处理。
                }

                FD_SET(fd_array[i], &readfds);              // 将有效的fd添加到readfds集合中，让select监控这些fd的读就绪状态

                if(fd_array[i] > maxfds)
                {
                    maxfds = fd_array[i];                   // 更新最大fd值
                    log_(Info,"最大文件描述符更新，最大文件描述符为：%d", maxfds);
                    std::cout << "最大文件描述符更新，最大文件描述符为：" << maxfds << std::endl;
                }
            }

            struct timeval timeout = {5, 0};

            // 调用select系统调用，监控maxfds+1个文件描述符（0到maxfds），readfds是读就绪集合，nullptr表示不监控写和异常
            int n = select(maxfds + 1, &readfds, NULL, NULL, &timeout); // 最后一个参数为nullptr表示不超时，阻塞等待
            // select 现在监控的是 0 到 maxfds 范围内所有在 readfds 位图中被置位的 fd。
            // 如果只有监听socket，它只监控监听socket。如果有多个连接，它会监控监听socket 和 所有已连接的socket。

            // 根据select返回值进行处理
            switch(n)
            {
            // select超时处理
            case 0:
                cout << "time out, timeout: " << timeout.tv_sec << "." << timeout.tv_usec << endl;
                break;
            // select调用出错处理
            case -1:
                cout << "select error, errno:" << errno << endl;
                break;
            // select成功处理
            default:
                cout << "得到了一个新的连接请求！" << endl;
                Dispatcher(readfds); // 调用Dispatcher处理所有就绪的事件
                break;
            }
        }
    }


private:
    Sock _listensock;                // 监听socket对象
    uint16_t _port;                  // 服务器端口号
    int fd_array[fd_num_max];        // 文件描述符数组，用于维护当前所有有效的连接fd（包括监听fd）
    // int wfd_array[fd_num_max];    // 扩展写事件处理
};