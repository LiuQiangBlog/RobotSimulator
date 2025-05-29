//
// Created by liuqiang on 25-5-29.
//
#include <json.hpp>
#include <Logging.h>

using json = nlohmann::json;

int main()
{
    std::string jsonStr = R"({"name": "Robot", "speed": 5.0})";
    json j = json::parse(jsonStr);

    std::string name = j["name"];
    double speed = j["speed"];

    std::cout << "Robot Name: " << name << ", Speed: " << speed << std::endl;
    return 0;
}
