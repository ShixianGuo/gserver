#ifndef __GS_LOG_FORMATTER_H__
#define __GS_LOG_FORMATTER_H__

#include "log.h"

namespace GS{
    class LogFormatter{
       public:
        typedef std::shared_ptr<LogFormatter> ptr;
        std::string format(LogEvent::ptr event);
    };
}

#endif