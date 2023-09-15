#include "logging.h"
#include "Timestamp.h"

#include <string.h>
#include <errno.h>

using namespace muduo;

thread_local char t_errnobuf[512];

// 创建全局唯一logger实例
Logger& Logger::getInstance() {
    static Logger logger;
    return logger;
}

// 设置日志等级
void Logger::setLogLevel(int logLevel) {
    logLevel_ = static_cast<Logger::LoggerLevel>(logLevel);
}

// 写日志：格式为时间 INFO 函数名 信息 文件名:行号
void Logger::log(std::string msg) {
    std::cout << Timestamp::now().toString();
    
    switch (this->logLevel_) {
        case Logger::LoggerLevel::INFO:
            std::cout << " [INFO]" <<  __FILE__ << ":" << __func__  << "," << __LINE__
            << ":"<< msg << std::endl; 
        case Logger::LoggerLevel::DEBUG:
            std::cout << " [DEBUG]" <<  __FILE__ << ":" << __func__  << "," << __LINE__
            << ":"<< msg << std::endl;
        case Logger::LoggerLevel::ERROR:
            std::cout << " [ERROR]" <<   __FILE__ << ":" << __func__  << "," << __LINE__
            << ":"<< msg << std::endl;
        case Logger::LoggerLevel::FATAL:
            std::cout << " [FATAL]" <<  __FILE__ << ":" << __func__  << "," << __LINE__
            << ":"<< msg << std::endl;
        default:
            break;
        
    }

}

// 错误输出相关
const char* muduo::strerror_tl(int savedErrno) {
    return ::strerror_r(savedErrno, t_errnobuf, sizeof t_errnobuf);
}