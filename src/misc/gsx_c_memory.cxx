

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "gsx_c_memory.h"

CMemory *CMemory::m_instance = NULL;

void *CMemory::AllocMemory(int memCount, bool ifmemset)
{
    void *tmpData = (void *)new char[memCount];
    if (ifmemset)
    {
        memset(tmpData, 0, memCount);
    }
    return tmpData;
}

void CMemory::FreeMemory(void *point)
{

    delete[]((char *)point);
}
