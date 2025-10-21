#include <iostream>
#include <fcntl.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
using namespace std;

int main()
{
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);  // 创建 TCP 套接字
    if (sockfd < 0)
    {
        perror("socket error");
        return -1;
    }

    // 获取当前文件状态标志
    int flags = fcntl(sockfd, F_GETFL, 0);
    if (flags < 0)
    {
        perror("fcntl F_GETFL error");
        return -1;
    }

    // 设置非阻塞模式（加上 O_NONBLOCK）
    if (fcntl(sockfd, F_SETFL, flags | O_NONBLOCK) < 0)
    {
        perror("fcntl F_SETFL error");
        return -1;
    }

    cout << "套接字已设置为非阻塞模式" << endl;

    close(sockfd);
    return 0;
}