//
// Created by liuqiang on 25-2-25.
//

#ifndef LOGGING_H
#define LOGGING_H
// #pragma warning(disable : 4996)

#include <chrono>
#include <cstring>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <fstream>
#include <string>
#include <vector>

#ifdef _WIN32
#define FILENAME (strrchr(__FILE__, '\\') + 1)
#else
#define FILENAME (strrchr(__FILE__, '/') + 1)
#endif

static std::string GetCurrentTimeStamp(int time_stamp_type = 0)
{
    std::chrono::system_clock::time_point now = std::chrono::system_clock::now();

    std::time_t now_time_t = std::chrono::system_clock::to_time_t(now);
    std::tm *now_tm = std::localtime(&now_time_t); // NOLINT

    char buffer[128];
    strftime(buffer, sizeof(buffer), "%F %T", now_tm);

    std::ostringstream ss;
    ss.fill('0');

    std::chrono::milliseconds ms;
    std::chrono::microseconds cs;
    std::chrono::nanoseconds ns;

    switch (time_stamp_type)
    {
    case 0: // s
        ss << buffer;
        break;
    case 1: // ms
        ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;
        ss << buffer << ":" << ms.count();
        break;
    case 2: // us
        ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;
        cs = std::chrono::duration_cast<std::chrono::microseconds>(now.time_since_epoch()) % 1000000;
        ss << buffer << ":" << std::setw(3) << ms.count() << std::setw(3) << cs.count() % 1000;
        break;
    case 3: // ns
        ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;
        cs = std::chrono::duration_cast<std::chrono::microseconds>(now.time_since_epoch()) % 1000000;
        ns = std::chrono::duration_cast<std::chrono::nanoseconds>(now.time_since_epoch()) % 1000000000;
        ss << buffer << ":" << ms.count() << ":" << cs.count() % 1000 << ":" << ns.count() % 1000;
        break;
    default:
        ss << buffer;
        break;
    }
    return ss.str();
}

#include <cstdint>

// usage:
// std::cout << clr::red     << " red "
//           << clr::yellow  << " yellow "
//           << clr::green   << " green "
//           << clr::cyan    << " cyan "
//           << clr::blue    << " blue "
//           << clr::magenta << " magenta "
//           << clr::grey    << " grey "
//           << clr::white   << " white "
//           << clr::reset   << " reset\n";
// std::cout << clr::red     << clr::on_cyan    << " red "
//           << clr::yellow  << clr::on_blue    << " yellow "
//           << clr::green   << clr::on_magenta << " green "
//           << clr::cyan    << clr::on_red     << " cyan "
//           << clr::blue    << clr::on_yellow  << " blue "
//           << clr::magenta << clr::on_green   << " magenta "
//           << clr::grey    << clr::on_white   << " grey "
//           << clr::white   << clr::on_grey    << " white "
//           << clr::reset                      << " reset\n";

#ifdef _WIN32
enum class clr : uint16_t
{
    grey,
    blue,
    green,
    cyan,
    red,
    magenta,
    yellow,
    white,
    on_blue,
    on_red,
    on_magenta,
    on_grey,
    on_green,
    on_cyan,
    on_yellow,
    on_white,
    reset = 0xFF
#elif __unix__
enum class clr : uint8_t
{
    grey = 30,
    red = 31,
    green = 32,
    yellow = 33,
    blue = 34,
    magenta = 35,
    cyan = 36,
    white = 37,
    on_grey = 40,
    on_red = 41,
    on_green = 42,
    on_yellow = 43,
    on_blue = 44,
    on_magenta = 45,
    on_cyan = 46,
    on_white = 47,
    reset
#else
#error unsupported
#endif
};

#ifdef _WIN32
namespace colored_cout_impl
{
inline uint16_t getColorCode(clr color);
inline uint16_t getConsoleTextAttr();
inline void setConsoleTextAttr(uint16_t attr);
} // namespace colored_cout_impl
#endif

template <typename type>
type &operator<<(type &ostream, const clr color)
{
#ifdef _WIN32
    // static const uint16_t initial_attributes = colored_cout_impl::getConsoleTextAttr();
    static const uint16_t initial_attributes = colored_cout_impl::getColorCode(clr::grey);
    static uint16_t background = initial_attributes & 0x00F0; // NOLINT
    static uint16_t foreground = initial_attributes & 0x000F; // NOLINT
#endif
    if (color == clr::reset)
    {
#ifdef _WIN32
        ostream.flush();
        colored_cout_impl::setConsoleTextAttr(initial_attributes);
        background = initial_attributes & 0x00F0; // NOLINT
        foreground = initial_attributes & 0x000F; // NOLINT
#elif __unix__
        ostream << "\033[m";
#endif
    }
    else
    {
#ifdef _WIN32
        uint16_t set = 0;
        const uint16_t colorCode = colored_cout_impl::getColorCode(color);
        if (colorCode & 0x00F0) // NOLINT
        {
            background = colorCode;
            set = background | foreground;
        }
        else if (colorCode & 0x000F) // NOLINT
        {
            foreground = colorCode;
            set = background | foreground;
        }
        ostream.flush();
        colored_cout_impl::setConsoleTextAttr(set);
#elif __unix__
        ostream << "\033[" << static_cast<uint32_t>(color) << "m";
#endif
    }
    return ostream;
}

#define MY_INFO (std::cout << clr::green << GetCurrentTimeStamp(2) << " " << FILENAME << ":" << __LINE__ << "] ")
#define MY_INFO_FILE                                                                                                \
    (FileLogger::GetOrCreateLogFile() << clr::green << GetCurrentTimeStamp(2) << " " << FILENAME << ":" << __LINE__ \
                                      << "] ")
#define MY_WARN (std::cout << clr::yellow << GetCurrentTimeStamp(2) << " " << FILENAME << ":" << __LINE__ << "] ")
#define MY_ERROR (std::cout << clr::red << GetCurrentTimeStamp(2) << " " << FILENAME << ":" << __LINE__ << "] ")

struct Logger
{
    ~Logger()
    {
        std::cout << clr::white << std::endl;
    }
};

struct FileLogger
{
    std::string fileName;
    std::ofstream &GetOrCreateLogFile()
    {
        static std::ofstream file(fileName, std::ofstream::app);
        std::cout << GetCurrentTimeStamp() + "_" + fileName << std::endl;
        return file;
    }
    void SetFileName(const std::string &name)
    {
        fileName = name;
        GetOrCreateLogFile();
    }

    ~FileLogger()
    {
        GetOrCreateLogFile() << clr::white << std::endl;
        GetOrCreateLogFile().close();
    }
};

template <typename T>
inline Logger &&operator<<(Logger &&wrap, T const &whatever)
{
    std::cout << whatever;
    return std::move(wrap);
}

template <typename T>
inline FileLogger &&operator<<(FileLogger &&wrap, T const &whatever)
{
    wrap.GetOrCreateLogFile() << whatever;
    return std::move(wrap);
}

#define LOG_INFO \
    (MY_INFO);   \
    Logger()
#define LOG_INFO_FILE \
    (MY_INFO_FILE);   \
    FileLogger()
#define LOG_WARN \
    (MY_WARN);   \
    Logger()
#define LOG_ERROR \
    (MY_ERROR);   \
    Logger()

#define CLOG_INFO LOG_INFO
#define CLOG_WARNING LOG_WARN
#define CLOG_ERROR LOG_ERROR

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN // Exclude rarely-used stuff from Windows headers
#define NOMINMAX            // Fixes the conflicts with STL

#include <Windows.h>
#include <wincon.h>

uint16_t colored_cout_impl::getColorCode(const clr color)
{
    switch (color)
    {
    case clr::grey:
        return FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED; // NOLINT
    case clr::blue:
        return FOREGROUND_BLUE | FOREGROUND_INTENSITY; // NOLINT
    case clr::green:
        return FOREGROUND_GREEN | FOREGROUND_INTENSITY; // NOLINT
    case clr::cyan:
        return FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_INTENSITY; // NOLINT
    case clr::red:
        return FOREGROUND_RED | FOREGROUND_INTENSITY; // NOLINT
    case clr::magenta:
        return FOREGROUND_BLUE | FOREGROUND_RED | FOREGROUND_INTENSITY; // NOLINT
    case clr::yellow:
        return FOREGROUND_GREEN | FOREGROUND_RED | FOREGROUND_INTENSITY; // NOLINT
    case clr::white:
        return FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED | FOREGROUND_INTENSITY; // NOLINT
    case clr::on_blue:
        return BACKGROUND_BLUE; //| BACKGROUND_INTENSITY
    case clr::on_red:
        return BACKGROUND_RED; //| BACKGROUND_INTENSITY
    case clr::on_magenta:
        return BACKGROUND_BLUE | BACKGROUND_RED; //| BACKGROUND_INTENSITY // NOLINT
    case clr::on_grey:
        return BACKGROUND_BLUE | BACKGROUND_GREEN | BACKGROUND_RED; // NOLINT
    case clr::on_green:
        return BACKGROUND_GREEN | BACKGROUND_INTENSITY; // NOLINT
    case clr::on_cyan:
        return BACKGROUND_BLUE | BACKGROUND_GREEN | BACKGROUND_INTENSITY; // NOLINT
    case clr::on_yellow:
        return BACKGROUND_GREEN | BACKGROUND_RED | BACKGROUND_INTENSITY; // NOLINT
    case clr::on_white:
        return BACKGROUND_BLUE | BACKGROUND_GREEN | BACKGROUND_RED | BACKGROUND_INTENSITY; // NOLINT
    case clr::reset:
    default:
        break;
    }
    return static_cast<uint16_t>(clr::reset);
}

uint16_t colored_cout_impl::getConsoleTextAttr()
{
    CONSOLE_SCREEN_BUFFER_INFO buffer_info;
    GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &buffer_info);
    return buffer_info.wAttributes;
}

void colored_cout_impl::setConsoleTextAttr(const uint16_t attr)
{
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), attr);
}

#endif // _WIN32

inline void CONSOLE_BRIDGE_logError(const char *str1, const char *str2 = "", const char *str3 = "")
{
    if (std::string(str2).empty())
    {
        LOG_ERROR << str1;
    }
    else
    {
        if (std::string(str3).empty())
        {
            LOG_ERROR << std::string(str1) + str2;
        }
        else
        {
            LOG_ERROR << std::string(str1) + str2 + ", " + str3;
        }
    }
}

inline void CONSOLE_BRIDGE_logWarn(const char *str1, const char *str2 = "", const char *str3 = "")
{
    if (std::string(str2).empty())
    {
        LOG_WARN << str1;
    }
    else
    {
        if (std::string(str3).empty())
        {
            LOG_WARN << std::string(str1) + str2;
        }
        else
        {
            LOG_WARN << std::string(str1) + str2 + ", " + str3;
        }
    }
}

inline void CONSOLE_BRIDGE_logInform(const char *str1, const char *str2 = "", const char *str3 = "")
{
    if (std::string(str2).empty())
    {
        LOG_INFO << str1;
    }
    else
    {
        if (std::string(str3).empty())
        {
            LOG_INFO << std::string(str1) + str2;
        }
        else
        {
            LOG_INFO << std::string(str1) + str2 + ", " + str3;
        }
    }
}

inline void CONSOLE_BRIDGE_logDebug(const char *str1, const char *str2 = "", const char *str3 = "")
{
    if (std::string(str2).empty())
    {
        LOG_INFO << str1;
    }
    else
    {
        if (std::string(str3).empty())
        {
            LOG_INFO << std::string(str1) + str2;
        }
        else
        {
            LOG_INFO << std::string(str1) + str2 + ", " + str3;
        }
    }
}
#endif // LOGGING_H
