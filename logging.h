#pragma once
#define MDDEBUG

#include "noncopyable.h"

#include <string>
#include <iostream>
#include <cstdio>
#include <cstdlib>

namespace muduo {
    /**
     * 日志类
    */
    class Logger : NonCopyable{     // 默认私有继承
    public:
        // 日志级别
        enum class LoggerLevel {
            INFO,
            DEBUG,
            ERROR,
            FATAL,  // core信息
        };
        // 创建全局唯一logger实例
        static Logger& getInstance();
        // 设置日志等级
        void setLogLevel(int logLevel);
        // 写日志
        void log(std::string msg);
    private:
        LoggerLevel logLevel_;

        Logger() {};
    };

    #ifdef MDDEBUG
    // ##__VA_ARGS__表示获取可变参数
    #define LOG_INFO(formatString, ...) \
    do {\
        Logger& logger = Logger::getInstance();\
        logger.setLogLevel(static_cast<int>(Logger::LoggerLevel::INFO));\
        char buffer[1024] = {0};\
        snprintf(buffer, 1024, formatString, ##__VA_ARGS__);\
    } while (0)
    #else
    #define LOG_INFO(formatString, ...)
    #endif

    #define LOG_DEBUG(formatString, ...)\
    do {\
        Logger& logger = Logger::getInstance();\
        logger.setLogLevel(static_cast<int>(Logger::LoggerLevel::DEBUG));\
        char buffer[1024] = {0};\
        snprintf(buffer, 1024, formatString, ##__VA_ARGS__);\
    } while (0)

    #define LOG_ERROR(formatString, ...)\
    do {\
        Logger& logger = Logger::getInstance();\
        logger.setLogLevel(static_cast<int>(Logger::LoggerLevel::ERROR));\
        char buffer[1024] = {0};\
        snprintf(buffer, 1024, formatString, ##__VA_ARGS__);\
    } while (0)

    #define LOG_FATAL(formatString, ...)\
    do {\
        Logger& logger = Logger::getInstance();\
        logger.setLogLevel(static_cast<int>(Logger::LoggerLevel::FATAL));\
        char buffer[1024] = {0};\
        snprintf(buffer, 1024, formatString, ##__VA_ARGS__);\
        exit(-1);\
    } while (0)

    const char* strerror_tl(int savedErrno);
}