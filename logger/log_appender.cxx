#include"log_appender.h"

namespace GS{
    void setFormatter(LogFormatter::ptr val){

    }
    void StdoutLogAppender::log(std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) {
        if(level >= m_level) {
            m_formatter->format(std::cout, logger, level, event);
        }
    }

    FileLogAppender::FileLogAppender(const std::string& filename)
       :m_filename(filename) {
        reopen();
    }

    void FileLogAppender::log(std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) {
        if(level >= m_level) {
            uint64_t now = event->getTime();
            if(now >= (m_lastTime + 3)) {
                reopen();
                m_lastTime = now;
            }
            //if(!(m_filestream << m_formatter->format(logger, level, event))) {
            if(!m_formatter->format(m_filestream, logger, level, event)) {
                std::cout << "error" << std::endl;
            }
        }
    }

    bool FileLogAppender::reopen(){
        if(m_filestream) m_filestream.close();

        m_filestream.open(m_filename);

        return !m_filestream;
    }
   
}