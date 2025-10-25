#pragma once

#include <unistd.h>
#include <fcntl.h>

// 将指定的socket设置为非阻塞模式，如果设置失败则退出程序
void SetNonBlockOrDie(int sock)
{
    int f1 = fcntl(sock, F_GETFL);              // 获取当前socket的文件状态标志
    if (f1 < 0)
    {
        exit(NON_BLOCK_ERR);                    // 如果获取失败，退出程序
    }

    fcntl(sock, F_SETFL, f1 | O_NONBLOCK);      // 设置socket为非阻塞模式
}