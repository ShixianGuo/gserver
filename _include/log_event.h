#ifndef __GS_LOG_EVENT_H__
#define __GS_LOG_EVENT_H__

#include"log.h"

namespace GS
{
    //日志事件
    class LogEvent{
        public:
            typedef std::shared_ptr<LogEvent> ptr;

            LogEvent(std::shared_ptr<Logger> logger
            ,LogLevel::Level level
            ,const char* file
            ,int32_t line
            ,uint32_t elapse
            ,uint32_t thread_id
            ,uint32_t fiber_id
            ,uint64_t time
            ,const std::string& thread_name);

        public:
            //返回文件名
            const char* getFile() const { return m_file;} 
            //返回行号
            int32_t getLine()const {return m_line;};
            //返回耗时
            uint32_t getElapse()const {return m_elapse;};
            //返回线程ID
            uint32_t getThreadId()const {return  m_threadId;};
            //返回协程ID
            uint32_t getFiberId()const {return m_fiberId;};
            //返回时间
            uint32_t getTime()const {return  m_time;};
            //返回线程名称
            const std::string& getThreadName() const { return m_threadName;};
            //返回日志内容
            const std::string& getContent()const{return m_ss.str();};
            //返回日志器
            std::shared_ptr<Logger> getLogger() const { return m_logger;}
            //返回日志级别
            LogLevel::Level getLevel() const { return m_level;}
            //返回日志内容字符串流
            std::stringstream& getSS() { return m_ss;}
            
            //格式化写入日志内容
            void format(const char* fmt, ...);
            void format(const char* fmt, va_list al);

        private: 
            /// 文件名
            const char* m_file = nullptr;
            /// 行号
            int32_t m_line = 0;
            /// 程序启动开始到现在的毫秒数
            uint32_t m_elapse = 0;
            /// 线程ID
            uint32_t m_threadId = 0;
            /// 协程ID
            uint32_t m_fiberId = 0;
            /// 时间戳
            uint64_t m_time = 0;
            /// 线程名称
            std::string m_threadName;
            /// 日志内容流
            std::stringstream m_ss;
            /// 日志器
            std::shared_ptr<Logger> m_logger;
            /// 日志等级
            LogLevel::Level m_level;
    };
}

#endif