//
// Created by liuqiang on 25-5-16.
//

#include <iostream>
#include <string>
#include <vector>
#include <regex>
#include <stdexcept>
#include <iomanip>

// 解析类似 "pos12-19" 的字符串，返回展开后的字符串列表
std::vector<std::string> expandRangeExpression(const std::string& expr) {
    std::vector<std::string> result;

    // 正则表达式模式：前缀 + 数字 + 连字符 + 数字
    std::regex pattern(R"(^([^\d]+)(\d+)-(\d+)$)");
    std::smatch matches;

    if (std::regex_match(expr, matches, pattern)) {
        if (matches.size() != 4) {
            throw std::invalid_argument("无效的范围表达式格式");
        }

        std::string prefix = matches[1].str();
        int start = std::stoi(matches[2].str());
        int end = std::stoi(matches[3].str());

        if (start > end) {
            throw std::invalid_argument("起始数字不能大于结束数字");
        }

        // 计算数字部分的宽度，用于前导零填充
        size_t width = matches[2].str().length();

        // 生成所有组合
        for (int i = start; i <= end; ++i) {
            std::stringstream ss;
            ss << prefix << std::setw(width) << std::setfill('0') << i;
            result.push_back(ss.str());
        }
    } else {
        // 如果不是范围表达式，直接返回原字符串
        result.push_back(expr);
    }

    return result;
}

int main() {
    try {
        std::string expr = "pos*90-109";
        std::vector<std::string> expanded = expandRangeExpression(expr);

        std::cout << "展开表达式: " << expr << std::endl;
        std::cout << "结果:" << std::endl;
        for (const auto& str : expanded) {
            std::cout << str << std::endl;
        }

        // 测试带前导零的情况
        expr = "ch01-05";
        expanded = expandRangeExpression(expr);

        std::cout << "\n展开表达式: " << expr << std::endl;
        std::cout << "结果:" << std::endl;
        for (const auto& str : expanded) {
            std::cout << str << std::endl;
        }

    } catch (const std::exception& e) {
        std::cerr << "错误: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}