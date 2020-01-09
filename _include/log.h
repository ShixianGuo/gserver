#ifndef __GS_LOG_H__
#define __GS_LOG_H__

#include<string>
#include<stdint.h>
#include<memory>
#include<list>

#include"log_appender.h"
#include"log_formatter.h"

namespace GS
{

 class LogEvent{
        public:
            typedef std::shared_ptr<LogEvent> ptr;
            LogEvent();
        private: 
            const char* m_file =nullptr;
            int32_t m_line=0;
            uint32_t m_elapse=0;
            uint32_t m_threadId=0;
            uint32_t m_fiberId=0
            uint64_t m_time;
            std::string m_content;
};

class LogLevel{
public: 
    enum Level{
      DEBUG =1,
      INFO =2,
      WARN=3,
      ERROR=4,
      FATAL=5
     };
};


class Logger{

public:
  typedef std::shared_ptr<Logger> ptr;
  

public:
  Logger(const std::string& name="root");
  void log(LogLevel::Level level,LogEvent::ptr event);

  void debug(LogEvent::ptr event);
  void info(LogEvent::ptr event);
  void warn(LogEvent::ptr event);
  void error(LogEvent::ptr event);
  void fatal(LogEvent::ptr event);

  void addAppender(LogAppender::ptr appender);
  void delAppender(LogAppender::ptr appender);

  LogLevel::level  getLevel(LogAppender::ptr appender) const {return m_level;}
  void setLevel(LogLevel::Level val){m_level=val;}
  
private:
   std::string m_name;
   LogLevel::Level m_level;
   std::list<LogAppender::ptr>  m_appenders;      //Appender集合

};





} // namespace nameG GS



#endif // !1__GS_LOG_H__
