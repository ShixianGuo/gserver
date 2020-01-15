#ifndef __GS_LOG_FORMATTER_H__
#define __GS_LOG_FORMATTER_H__

#include "log.h"

namespace GS{

    /**
     *  %m 消息
     *  %p 日志级别
     *  %r 累计毫秒数
     *  %c 日志名称
     *  %t 线程id
     *  %n 换行
     *  %d 时间
     *  %f 文件名
     *  %l 行号
     *  %T 制表符
     *  %F 协程id
     *  %N 线程名称
     *
     *  默认格式 "%d{%Y-%m-%d %H:%M:%S}%T%t%T%N%T%F%T[%p]%T[%c]%T%f:%l%T%m%n"
     */

    //日志格式化
    class LogFormatter{
       public:
            typedef std::shared_ptr<LogFormatter> ptr;
            LogFormatter(const std::string& pattern);

       public:
            //返回格式化日志文本
            std::string format(std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event);
            std::ostream& format(std::ostream& ofs, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event);
      
       public:
            class FormatItem{
                public:
                    typedef std::shared_ptr<FormatItem> ptr;
                    virtual ~FormatItem(){}
                public:    
                    virtual void format(std::ostream& os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) = 0;
            };
            //初始化,解析日志模板
            void init();
            //是否有错误
            bool isError() const { return m_error;}
            //返回日志模板
            const std::string getPattern() const { return m_pattern;}
      
       private:
            /// 日志格式模板
            std::string m_pattern;
            /// 日志格式解析后格式
            std::vector<FormatItem::ptr> m_items;
            /// 是否有错误
            bool m_error = false;
      

    };

    
}

#endif