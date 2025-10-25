#pragma once
#include <iostream>
#include "Protocol.hpp"

enum
{
    Div_Zero = 1,
    Mod_Zero,
    Other_Oper
};

class Server_Cal
{
public:
    Server_Cal()
    {

    }
    ~Server_Cal()
    {

    }

    Response CalculatorHelper(const Request& req)
    {
        Response resp(0, 0);
        switch (req.op_)
        {
            case '+':
                resp.result = req.x_ + req.y_;
                break;
            case '-':
                resp.result = req.x_ - req.y_;
                break;
            case '*':
                resp.result = req.x_ * req.y_;
                break;
            case '/':
            {
                if (req.y_ == 0)
                {
                    resp.code = Div_Zero;
                }
                else
                {
                    resp.result = req.x_ / req.y_;
                }
            }
            break;
            case '%':
            {
                if (req.y_ == 0)
                {
                    resp.code = Mod_Zero;
                }
                else
                {
                    resp.result = req.x_ % req.y_;
                }
            }
            break;
            default:
                resp.code = Other_Oper;
                break;
        }

        return resp;
    }
    // "len"\n"10 + 20"\n
    // string Calculator(string& package)
    // {
    //     string content;
    //     bool r = Decode(package, &content); // "len"\n"10 + 20"\n
    //     if (!r)
    //     {
    //         return "";
    //     }

    //     // "10 + 20"
    //     Request req;
    //     r = req.Deserialize(content); // "10 + 20" ->x=10 op=+ y=20
    //     if (!r)
    //     {
    //         return "";
    //     }

    //     content = "";                          //
    //     Response resp = CalculatorHelper(req); // result=30 code=0;

    //     resp.Serialize(&content);  // "30 0"
    //     content = Encode(content); // "len"\n"30 0"

    //     return content;
    // }

    string Calculator(string& package)
    {
        // cout << "[DEBUG] Calculator被调用，package内容长度: " << package.length() << endl;
        // cout << "[DEBUG] package内容: [" << package << "]" << endl;
        
        string content;
        bool r = Decode(package, &content);
        if (!r)
        {
            // cout << "[DEBUG] Decode失败" << endl;
            return "";
        }
        
        // cout << "[DEBUG] Decode成功，content: [" << content << "]" << endl;

        Request req;
        r = req.Deserialize(content);
        if (!r)
        {
            // cout << "[DEBUG] Deserialize失败" << endl;
            return "";
        }
        
        // cout << "[DEBUG] 请求解析成功: " << req.x_ << " " << req.op_ << " " << req.y_ << endl;

        Response resp = CalculatorHelper(req);
        
        // cout << "[DEBUG] 计算完成，结果: " << resp.result << ", 状态码: " << resp.code << endl;

        content = "";
        resp.Serialize(&content);
        content = Encode(content);

        // cout << "[DEBUG] 响应编码完成: [" << encoded_content << "]" << endl;
        return content;
    }

};