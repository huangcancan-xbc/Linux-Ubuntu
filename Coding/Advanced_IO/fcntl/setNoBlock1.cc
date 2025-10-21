#include <iostream>
#include <unistd.h>
#include <errno.h>
#include <cstring>
#include <fcntl.h>
using namespace std;

void setNoBlock(int fd)
{
    int fl = fcntl(fd, F_GETFL);                // 获取原来文件描述符的属性
    if(fl < 0)
    {
        perror("fcntl");
        return;
    }

    fcntl(fd, F_GETFL, fl | O_NONBLOCK);        // 在原标志基础上加上非阻塞
    cout << "设置 " << fd << " 为非阻塞模式成功！" << endl;
}

int main()
{
    char buffer[1024];

    setNoBlock(0);

    while(true)
    {
        // printf("please enter: ");
        // fflush(stdout);

        ssize_t n = read(0, buffer, sizeof(buffer) - 1);
        if(n > 0)
        {
            buffer[n - 1] = '\0';
            cout << "echo: " << buffer << endl;
        }
        else if(n == 0)
        {
            cout << "read EOF/read done!" << endl;
        }
        else
        {
            cerr << "read error, n = " << n << "error code: " << errno << "error string: " << strerror(errno) << endl;
        }
    }

    return 0;
}