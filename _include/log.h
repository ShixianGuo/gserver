#ifndef __GS_LOG_H__
#define __GS_LOG_H__

#include<string>
#include<stdint.h>
#include<memory>
#include<list>
#include<vector>
#include<sstream>
#include<fstream>
#include<iostream>
#include<functional>
#include<time.h>
#include<stdarg.h>
#include<map>
#include"log_appender.h"
#include"log_formatter.h"
#include"log_event.h"
#include"log_level.h"

namespace GS
{

  class Logger: public std::enable_shared_from_this<Logger> {
    friend class LogAppender;
    public:
      typedef std::shared_ptr<Logger> ptr;

    public:
      //构造函数
      Logger(const std::string& name="root");
      //写日志
      void log(LogLevel::Level level ,LogEvent::ptr event);
      
      //写对应级别日志
      void debug(LogEvent::ptr event);
      void info(LogEvent::ptr event);
      void warn(LogEvent::ptr event);
      void error(LogEvent::ptr event);
      void fatal(LogEvent::ptr event);
      
      //添加日志目标
      void addAppender(LogAppender::ptr appender);
      //删除日志目标
      void delAppender(LogAppender::ptr appender);
      //清空日志目标
      void clearAppenders();
      
      //返回日志级别
      LogLevel::Level  getLevel(LogAppender::ptr appender) const {return m_level;}
      //设置日志级别
      void setLevel(LogLevel::Level val){m_level=val;}

      //返回日志名称
      const std::string& getName() const { return m_name;}
      
      //设置日志格式器
      void setFormatter(LogFormatter::ptr val);
      //设置日志格式模板
      void setFormatter(const std::string& val);
      //获取日志格式器
      LogFormatter::ptr getFormatter();

    private:
      /// 日志名称
      std::string m_name;
      /// 日志级别
      LogLevel::Level m_level;
      /// 日志目标集合
      std::list<LogAppender::ptr> m_appenders;
      /// 日志格式器
      LogFormatter::ptr m_formatter;
      /// 主日志器
      Logger::ptr m_root;

  };



} // namespace nameG GS



#endif // !1__GS_LOG_H__
