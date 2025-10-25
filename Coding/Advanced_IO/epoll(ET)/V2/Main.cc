#include <iostream>
#include <functional>
#include "TcpServer.hpp"
#include "Log.hpp"
#include "Server_Cal.hpp"
#include <memory>

Server_Cal server_cal;                      // 创建一个全局的计算器对象实例

// 处理客户端消息的回调函数，接收weak_ptr类型的连接对象
void DefaultOnMessage(std::weak_ptr<Connection> conn)
{
    if(conn.expired())                      // 检查连接对象是否已经过期（被销毁）
    {
        return;
    }

    auto connection_ptr = conn.lock();      // 将weak_ptr转换为shared_ptr以安全访问连接对象

    // std::cout << "上层得到了数据： " << connection_ptr->Inbuffer() << std::endl;
    // std::string response_str = Server_Cal.CalculatorHelper(connection_ptr->Inbuffer());
    // 使用计算器对象处理客户端发送的计算请求字符串
    std::string response_str = server_cal.Calculator(connection_ptr->Inbuffer());

    if(response_str.empty())
    {
        return;
    }

    log_(Debug, "%s", response_str.c_str());
    // response_str 发送出去
    // 将响应数据添加到连接的输出缓冲区
    connection_ptr->AppendOutBuffer(response_str);
    
    auto tcpserver = connection_ptr->_tcp_server_ptr.lock();    // 获取TCP服务器的共享指针
    tcpserver->Sender(connection_ptr);                          // 直接调用发送函数处理数据发送
}

int main()
{
    // std::unique_ptr<TcpServer> epoll_svr(new TcpServer(8080));
    // epoll_svr->Init();
    // epoll_svr->Loop();

    // 创建TCP服务器对象，端口为8080，消息处理函数为DefaultOnMessage
    std::shared_ptr<TcpServer> epoll_svr(new TcpServer(8080, DefaultOnMessage));
    epoll_svr->Init();              // 初始化服务器
    epoll_svr->Loop();              // 启动服务器事件循环

    return 0;
}