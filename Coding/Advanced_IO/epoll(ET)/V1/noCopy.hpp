#pragma once

// 防拷贝基类，继承这个类的子类都不能被拷贝
class noCopy
{
public:
    noCopy() = default;                             // 默认构造函数，让编译器自动生成
    noCopy(const noCopy &) = delete;                // 删除拷贝构造函数，禁止拷贝
    const noCopy &operator=(const noCopy &) = delete;   // 删除赋值运算符，禁止赋值
};