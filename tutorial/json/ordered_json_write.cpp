//
// Created by liuqiang on 25-5-29.
//
#include <iostream>
#include <fstream>
#include <json.hpp>

using ordered_json = nlohmann::ordered_json;
using json = nlohmann::json;

int main()
{
    // 创建一个 ordered_json 对象
    ordered_json j;
//    json j;
    j["first"] = 1;
    j["second"] = 2;
    j["third"] = 3;
    j["a"] = 3;

    // 将 ordered_json 对象写入文件
    std::ofstream file("config.json");
    if (file.is_open())
    {
        file << j.dump(4); // 4 表示缩进为 4 个空格，用于格式化输出
        file.close();
        std::cout << "JSON 已写入文件 config.json" << std::endl;
    }
    else
    {
        std::cerr << "无法打开文件" << std::endl;
    }

    return 0;
}