#pragma once

#include <iostream>
#include <string>
#include <stdlib.h>                                      // exit, perror
#include <unistd.h>                                      // read, write, close
#include <sys/types.h>                                   // open, close, read, write, lseek
#include <sys/stat.h>                                    // mkdir
#include <fcntl.h>                                       // open, O_RDONLY, O_WRONLY, O_CREAT, O_APPEND
#include <errno.h>                                       // errno
#include <sys/time.h>                                    // gettimeofday, struct timeval
#include <ctime>                                         // localtime_r, struct tm
#include <cstdarg>                                       // va_list, va_start, va_end
using namespace std;

// 管道错误码
enum FIFO_ERROR_CODE
{
    FIFO_CREATE_ERR = 1,                                 // 这是创建管道文件失败的错误码
    FIFO_DELETE_ERR = 2,                                 // 这是删除管道文件失败的错误码
    FIFO_OPEN_ERR                                        // 这是打开管道文件失败的错误码（枚举会自动赋值为3）
};

// 日志等级
enum Log_Level
{
    Fatal,                                               // 最严重级别
    Error,                                               // 严重错误
    Warning,                                             // 警告
    Debug,                                               // 调试信息
    Info                                                 // 普通信息
};

class Log
{
    int  enable = 1;                                     // 是否启用日志
    int  classification = 1;                             // 是否分类
    string log_path = "./log.txt";                       // 日志存放路径
    int  console_out = 1;                                // 是否输出到终端

    // 日志等级转换成字符串
    string level_to_string(int level)
    {
        switch (level)
        {
            case Fatal:
                return "Fatal";
            case Error:
                return "Error";
            case Warning:
                return "Warning";
            case Debug:
                return "Debug";
            case Info:
                return "Info";
            default:
                return "None";
        }
    }

    // 获取当前计算机的时间，返回格式：YYYY-MM-DD HH:MM:SS.UUUUUU （含微秒）
    string get_current_time()
    {
        struct timeval tv;                               // timeval：包含秒和微秒
        gettimeofday(&tv, nullptr);                      // 系统调用：获取当前时间（精确到微秒）

        struct tm t;                                     // tm：分解时间，转格式（年、月、日、时、分、秒）
        localtime_r(&tv.tv_sec, &t);                     // 把秒转换成年月日时分秒（本地时区）

        char buffer[64];                                 // 定义字符数组作为格式化输出的缓冲区

        snprintf(buffer, sizeof(buffer),
                "%04d-%02d-%02d %02d:%02d:%02d.%06ld",
                t.tm_year + 1900,                        // 年：tm_year 从 1900 开始计数
                t.tm_mon + 1,                            // 月：tm_mon 从 0 开始，0 表示 1 月
                t.tm_mday,                               // 日
                t.tm_hour,                               // 时
                t.tm_min,                                // 分
                t.tm_sec,                                // 秒
                tv.tv_usec);                             // 微秒部分，取自 gettimeofday

        return string(buffer);                           // 转换成 string 返回
    }

public:
    Log() = default;                                     // 使用默认构造
    Log(int enable, int classification, string log_path, int console_out)
        : enable(enable),
        classification(classification),
        log_path(log_path),
        console_out(console_out)
    {

    }
    
    // 重载函数调用运算符
    void operator()(int level, const string& format, ...)
    {
        if (enable == 0)
        {
            return;                                      // 日志未启用
        }

        va_list args;
        va_start(args, format);
        
        // 计算需要的缓冲区大小
        int size = vsnprintf(nullptr, 0, format.c_str(), args) + 1;
        va_end(args);
        
        if (size <= 0)
        {
            return;                                      // 格式化失败
        }
        
        // 分配缓冲区并格式化字符串
        char* buffer = new char[size];
        va_start(args, format);
        vsnprintf(buffer, size, format.c_str(), args);
        va_end(args);
        
        string content(buffer);
        delete[] buffer;
        
        // 调用原有的处理逻辑
        string level_str = "[" + level_to_string(level) + "] ";
        string log_message;

        if (classification == 1)
        {
            log_message = level_str + "[" + get_current_time() + "] " + content + "\n";
        }
        else if (classification == 0)
        {
            log_message = "[" + get_current_time() + "] " + content + "\n";
        }
        else
        {
            printf("传入的分类参数错误！\n");
            return;
        }

        if (console_out == 1)
        {
            cout << log_message;
        }

        log_to_file(level, log_message);
    }

private:
    // 文件路径的后缀处理函数：当按照日志等级分类存储并且文件路径是 "./log.txt" 这种有文件扩展名时的处理方法
    string Suffix_processing(int level, string log_path)
    {
        string Path;
        if (log_path.back() == '/')                      // 如果是一个目录的路径，比如 "./log/"，则最终文件名为 "log_等级名.txt"
        {
            Path = log_path + "log_" + level_to_string(level) + ".txt";
        }
        else                                             // 如果是一个文件路径，比如 "./log.txt"，则最终文件名为 "log_等级名.txt"
        {
            size_t pos = log_path.find_last_of('.');     // 从后往前找到第一个 '.' 的位置，即最后一次出现的 '.' 的位置
            if (pos != string::npos)
            {
                string left = log_path.substr(0, pos);   // 去掉后缀，即我所需要的有效的前部分路径
                string right = log_path.substr(pos);     // 保留后缀，即有效的文件扩展名
                Path = left + "_" + level_to_string(level) + right;         // 组合成新的文件名
            }
            else                                         // 如果没有文件扩展名（比如 "./log"），则直接在文件名后面加上 "_等级名.txt"
            {
                Path = log_path + "_" + level_to_string(level) + ".txt";
            }
        }

        return Path;
    }

    // 核心写文件函数
    void log_to_file(int level, const string& log_content)
    {
        string Path;

        if (classification == 1)
        {
            Path = Suffix_processing(level, log_path);   // 按照日志等级分类存储
        }
        else if (classification == 0)
        {
            Path = log_path;                             // 不分类直接使用传入的 log_path
        }

        // 追加写入，文件不存在则创建，权限 0644
        int fd = open(Path.c_str(), O_WRONLY | O_CREAT | O_APPEND, 0644);
        if (fd < 0)
        {
            perror("");
            exit(FIFO_OPEN_ERR);
        }

        write(fd, log_content.c_str(), log_content.size());
        close(fd);
    }
};

extern Log log_;