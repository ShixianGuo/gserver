

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <unistd.h>

#include "gsx_func.h"
#include "gsx_macro.h"
#include "gsx_c_conf.h"

void gsx_process_events_and_timers()
{
    g_socket.gsx_epoll_process_events(-1);
}
