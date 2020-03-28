
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "gsx_global.h"

void gsx_init_setproctitle()
{

    gp_envmem = new char[g_envneedmem];
    memset(gp_envmem, 0, g_envneedmem);

    char *ptmp = gp_envmem;

    for (int i = 0; environ[i]; i++)
    {
        size_t size = strlen(environ[i]) + 1;
        strcpy(ptmp, environ[i]);
        environ[i] = ptmp;
        ptmp += size;
    }
    return;
}

void gsx_setproctitle(const char *title)
{

    size_t ititlelen = strlen(title);

    size_t esy = g_argvneedmem + g_envneedmem;
    if (esy <= ititlelen)
    {

        return;
    }

    g_os_argv[1] = NULL;

    char *ptmp = g_os_argv[0];
    strcpy(ptmp, title);
    ptmp += ititlelen;

    size_t cha = esy - ititlelen;
    memset(ptmp, 0, cha);
    return;
}