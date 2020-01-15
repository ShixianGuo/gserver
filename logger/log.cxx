#include"log.h"

namespace GS{

    Logger::Logger(const std::string& name)
        :m_name(name)
        ,m_level(LogLevel::DEBUG) {
        m_formatter.reset(new LogFormatter("%d{%Y-%m-%d %H:%M:%S}%T%t%T%N%T%F%T[%p]%T[%c]%T%f:%l%T%m%n"));
    }


    void Logger::log(LogLevel::Level level, LogEvent::ptr event) {
        if(level >= m_level) {
            auto self = shared_from_this();
            if(!m_appenders.empty()) {
                for(auto& i : m_appenders) {
                    i->log(self, level, event);
                }
            } else if(m_root) {
                m_root->log(level, event);
            }
        }
    }

    void Logger::debug(LogEvent::ptr event) {
        log(LogLevel::DEBUG, event);
    }

    void Logger::info(LogEvent::ptr event) {
        log(LogLevel::INFO, event);
    }

    void Logger::warn(LogEvent::ptr event) {
        log(LogLevel::WARN, event);
    }

    void Logger::error(LogEvent::ptr event) {
        log(LogLevel::ERROR, event);
    }

    void Logger::fatal(LogEvent::ptr event) {
        log(LogLevel::FATAL, event);
    }

    void Logger::addAppender(LogAppender::ptr appender) {
        if(!appender->getFormatter()) {
            appender->m_formatter = m_formatter;
        }
        m_appenders.push_back(appender);
    }
    void Logger::delAppender(LogAppender::ptr appender) {
        for(auto it = m_appenders.begin();
                it != m_appenders.end(); ++it) {
            if(*it == appender) {
                m_appenders.erase(it);
                break;
            }
        }
    }
    void Logger::clearAppenders() {
        m_appenders.clear();
    }

    void Logger::setFormatter(LogFormatter::ptr val) {
        m_formatter = val;

        for(auto& i : m_appenders) {
            if(!i->m_hasFormatter) {
                i->m_formatter = m_formatter;
            }
        }
    }

    void Logger::setFormatter(const std::string& val) {
        std::cout << "---" << val << std::endl;
        GS::LogFormatter::ptr new_val(new GS::LogFormatter(val));
        if(new_val->isError()) {
            std::cout << "Logger setFormatter name=" << m_name
                      << " value=" << val << " invalid formatter"
                      << std::endl;
            return;
        }
        //m_formatter = new_val;
        setFormatter(new_val);
    }


    LogFormatter::ptr Logger::getFormatter() {
        return m_formatter;
    }


}