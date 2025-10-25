#include <iostream>
#include <string>
#include "Socket.hpp"
#include "Protocol.hpp"
using namespace std;

void Usage(string proc)
{
    cout << "\n\r用法: " << proc << " IP 端口号\n" << endl;
    cout << "示例：" << proc << " 127.0.0.1 8080\n" << endl;
}

int main(int argc, char *argv[])
{
    if(argc != 3)
    {
        Usage(argv[0]);
        exit(0);
    }

    string server_ip = argv[1];
    uint16_t server_port = atoi(argv[2]);

    // 创建套接字并连接服务器
    Sock sock;
    sock.Socket();
    bool flag = sock.Connect(server_ip, server_port);

    if(!flag)
    {
        cerr << "连接服务器失败!" << endl;
        return 1;
    }

    cout << "连接服务器成功! 请输入计算表达式 ，输入quit退出" << endl;

    // 发送数据
    while(true)
    {
        string message;
        cout << "请输入计算表达式# ";
        getline(cin, message);
        
        if(message == "quit")
        {
            break;
        }

        // 解析输入：查找操作符
        size_t op_pos = string::npos;
        char op = 0;
        
        for(size_t i = 0; i < message.length(); i++)
        {
            if(message[i] == '+' || message[i] == '-' || message[i] == '*' || 
               message[i] == '/' || message[i] == '%')
            {
                op_pos = i;
                op = message[i];
                break;
            }
        }
        
        if(op_pos == string::npos)
        {
            cout << "无效的表达式格式! 请使用格式: 数字+操作符+数字" << endl;
            continue;
        }
        
        // 提取操作数
        string x_str = message.substr(0, op_pos);
        string y_str = message.substr(op_pos + 1);
        
        try {
            long long x = stoll(x_str);
            long long y = stoll(y_str);
            
            // 构造请求对象并编码
            Request req(x, op, y);
            string content;
            req.Serialize(&content);
            string package = Encode(content);
            
            // 发送数据
            int n = write(sock.Fd(), package.c_str(), package.size());
            if (n < 0)
            {
                cerr << "发送数据失败！" << endl;
                break;
            }

            // 接收响应 - 直接解析服务器返回的格式
            char inbuf[4096];
            n = read(sock.Fd(), inbuf, sizeof(inbuf)-1);
            if (n > 0)
            {
                inbuf[n] = '\0';
                string response_package = inbuf;

                // 直接解析服务器返回的格式：4\n24 0\n
                // 手动解析格式
                size_t first_newline = response_package.find('\n');
                if(first_newline != string::npos)
                {
                    size_t second_newline = response_package.find('\n', first_newline + 1);
                    if(second_newline != string::npos)
                    {
                        // 提取中间的内容：24 0
                        string content_part = response_package.substr(first_newline + 1, 
                                                                    second_newline - first_newline - 1);
                        
                        // 手动解析结果和错误码
                        size_t space_pos = content_part.find(' ');
                        if(space_pos != string::npos)
                        {
                            string result_str = content_part.substr(0, space_pos);
                            string code_str = content_part.substr(space_pos + 1);
                            
                            try {
                                long long result = stoll(result_str);
                                int code = stoi(code_str);
                                cout << "结果是" << result << "，错误码：" << code << endl;
                            }
                            catch(const exception& e)
                            {
                                cout << "解析错误: " << e.what() << endl;
                            }
                        }
                        else
                        {
                            cout << "格式错误：缺少空格分隔符" << endl;
                        }
                    }
                    else
                    {
                        cout << "格式错误：缺少第二个换行符" << endl;
                    }
                }
                else
                {
                    cout << "格式错误：缺少换行符" << endl;
                }
            }
            else
            {
                cout << "服务器无响应" << endl;
                break;
            }
        }
        catch(const exception& e)
        {
            cout << "输入格式错误，请输入数字!" << endl;
        }
    }

    sock.Close();
    return 0;
}