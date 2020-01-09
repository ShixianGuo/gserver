#include"log.h"

namespace GS{


  Logger::Logger(const std::string& name="root"){
      
  }

  void Logger::log(LogLevel::Level level,LogEvent::ptr event);

  void Logger::debug(LogEvent::ptr event){

  }
  void Logger::info(LogEvent::ptr event){
      
  }
  void Logger::warn(LogEvent::ptr event){
      
  }
  void Logger::error(LogEvent::ptr event){
      
  }
  void Logger::fatal(LogEvent::ptr event){
      
  }

  void Logger::addAppender(LogAppender::ptr appender){
      m_appenders.push_back(appender);
  }
  void Logger::delAppender(LogAppender::ptr appender){
      
  }

}