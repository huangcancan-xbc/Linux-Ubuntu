#pragma once

#include <iostream>
#include <string>

using namespace std;


const string Space_delimiter = " ";                                   // 空格分隔符
const string protocol_sep = "\n";                                     // 协议分隔符

string Encode(string &content)
{
    string package = to_string(content.size());                       // 内容长度
    package += protocol_sep;                                          // 添加分隔符
    package += content;                                               // 添加内容
    package += protocol_sep;                                          // 添加分隔符

    return package;
}

// "len"\n"x op y"\n
bool Decode(string &package, string *content)
{
    // 查找第一个换行符（长度字段结束）
    size_t pos = package.find(protocol_sep);
    if(pos == string::npos)
    {
        return false;
    }
    
    // 提取长度字符串
    string len_str = package.substr(0, pos);
    size_t len = stoll(len_str);  // 使用 stoll 而不是 stoll
    
    // 计算总长度：长度字段 + 内容长度 + 2个换行符
    size_t total_len = len_str.size() + len + 2;
    if(package.size() < total_len)
    {
        return false;
    }

    // 提取有效内容
    *content = package.substr(pos + 1, len);
    
    // 从原始包中移除已处理的数据
    package.erase(0, total_len);

    return true;
}

// 请求类
class Request
{
public:
    long long x_;                                                     // 第一个操作数
    char op_;                                                         // 操作符
    long long y_;                                                     // 第二个操作数

public:
    Request(long long x, char op, long long y) : x_(x), op_(op), y_(y) {}
    Request() {}

    // 序列化函数
    bool Serialize(string *out)
    {
        string s = to_string(x_) + Space_delimiter + op_ + Space_delimiter + to_string(y_);
        *out = s;
        return true;
    }

    // 反序列化函数
    bool Deserialize(string &in)
    {
        ssize_t left = in.find(Space_delimiter);
        if(left == string::npos)
        {
            return false;
        }

        string part_x = in.substr(0, left);

        ssize_t right = in.rfind(Space_delimiter);
        if(right == string::npos)
        {
            return false;
        }

        string part_y = in.substr(right + 1);

        if(left + 2 != right)
        {
            return false;
        }

        x_ = stoll(part_x);
        op_ = in[left + 1];
        y_ = stoll(part_y);

        return true;
    }

    void Print()
    {
        cout << "构建完成：" << x_ << op_ << y_ << "= ?"<< endl;
    }
};




// 响应类
class Response
{
public:
    long long result;                                               // 计算结果
    int code;                                                       // 状态码：0表示成功，非0表示具体错误类型

public:
    Response(long long result, int code) : result(result), code(code) {}
    Response() {}

    // 序列化函数
    bool Serialize(string *out)
    {
        string s = to_string(result) + Space_delimiter + to_string(code);
        *out = s;
        return true;
    }

    // 反序列化函数
    bool Deserialize(string &in)
    {
        ssize_t left = in.find(Space_delimiter);
        if(left == string::npos)
        {
            return false;
        }

        string part_result = in.substr(0, left);

        ssize_t right = in.rfind(Space_delimiter);
        if(right == string::npos)
        {
            return false;
        }

        string part_code = in.substr(right + 1);

        if(left + 2 != right)
        {
            return false;
        }

        result = stoll(part_result);
        code = stoll(part_code);

        return true;
    }

    void Print()
    {
        if(code == 0)
        {
            cout << "计算结果：" << result << endl;
        }
        else
        {
            cout << "计算失败：" << code << endl;
        }
    }
};