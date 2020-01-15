#ifndef __GS_LOG_LEVEL_H__
#define __GS_LOG_LEVEL_H__

#include"log.h"

namespace GS
{
//日志级别
class LogLevel{
public: 
    enum Level {
        /// 未知级别
        UNKNOW = 0,
        /// DEBUG 级别
        DEBUG = 1,
        /// INFO 级别
        INFO = 2,
        /// WARN 级别
        WARN = 3,
        /// ERROR 级别
        ERROR = 4,
        /// FATAL 级别
        FATAL = 5
    };

public: 
     //将日志级别转成文本输出
     static const char* ToStirng(LogLevel::Level level);
     //将文本转换成日志级别
     static LogLevel::Level FromString(const std::string& str);
};
}

#endif