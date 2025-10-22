#include <iostream>
#include <sys/select.h>
#include <unistd.h>
using namespace std;

int main()
{
    fd_set readfds;                  // 创建fd集合
    FD_ZERO(&readfds);               // 清空集合
    FD_SET(STDIN_FILENO, &readfds);  // 把标准输入加入监控集合

    struct timeval tv;
    tv.tv_sec = 5;                   // 最多阻塞5秒
    tv.tv_usec = 0;

    cout << "等待输入中..." << endl;
    int ret = select(STDIN_FILENO + 1, &readfds, NULL, NULL, &tv);

    if (ret > 0)
    {
        if (FD_ISSET(STDIN_FILENO, &readfds))
        {
            cout << "检测到输入：" << endl;
            char buf[1024];
            read(STDIN_FILENO, buf, sizeof(buf));
            cout << "你输入了: " << buf;
        }
    }
    else if (ret == 0)
    {
        cout << "超时，没有输入" << endl;
    }
    else
    {
        perror("select 出错");
    }

    return 0;
}