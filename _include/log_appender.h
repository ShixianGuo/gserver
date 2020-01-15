#ifndef __GS_LOG_APPENDER_H__
#define __GS_LOG_APPENDER_H__

#include"log.h"

namespace GS{
    //日志输出目标
    class LogAppender{
        public:
            typedef std::shared_ptr<LogAppender> ptr;
            virtual ~LogAppender(){}

        public:
            //写入日志
            virtual void log(std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) = 0;
            //更改日志格式器
            void setFormatter(LogFormatter::ptr val);
            //获取日志格式器
            LogFormatter::ptr getFormatter()const{return m_formatter;};
            //获取日志级别
            LogLevel::Level getLevel() const { return m_level;}
            //设置日志级别
            void setLevel(LogLevel::Level val) { m_level = val;}
        public:
            /// 日志级别
            LogLevel::Level m_level = LogLevel::DEBUG;
            /// 是否有自己的日志格式器
            bool m_hasFormatter = false;
            /// 日志格式器
            LogFormatter::ptr m_formatter;
            
    };
    //输出到控制台的Appender
    class StdoutLogAppender:public LogAppender{
        public:
            typedef std::shared_ptr<StdoutLogAppender> ptr;
            void log(std::shared_ptr<Logger> logger,LogLevel::Level level,LogEvent::ptr envnt) override;
    };
    //输出到文件的Appender
    class FileLogAppender:public LogAppender{
        public:
            typedef std::shared_ptr<FileLogAppender> ptr;
            FileLogAppender(const std::string&fliename);

            void log(std::shared_ptr<Logger> logger,LogLevel::Level level,LogEvent::ptr envnt) override;
            
            bool reopen();
           
        private:
            /// 文件路径
            std::string m_filename;
            /// 文件流
            std::ofstream m_filestream;
            /// 上次重新打开时间
            uint64_t m_lastTime = 0;
    };

    
}

#endif // !1__GS_LOG_APPENDER_H__