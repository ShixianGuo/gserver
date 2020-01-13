#ifndef __GS_LOG_APPENDER_H__
#define __GS_LOG_APPENDER_H__

#include"log.h"

namespace GS{
    
    class LogAppender{
        public:
            typedef std::shared_ptr<LogAppender> ptr;
            virtual ~LogAppender(){}
            
    };

    class StdoutLogAppender:public LogAppender{

    };

    
}

#endif // !1__GS_LOG_APPENDER_H__