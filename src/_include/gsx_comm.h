
#ifndef __GSX_COMM_H__
#define __GSX_COMM_H__

#define _PKG_MAX_LENGTH 30000
#define _PKG_HD_INIT 0 
#define _PKG_HD_RECVING 1 
#define _PKG_BD_INIT 2 
#define _PKG_BD_RECVING 3
#define _DATA_BUFSIZE_ 20
#pragma pack(1)
typedef struct _COMM_PKG_HEADER
{
	unsigned short pkgLen;
	unsigned short msgCode;
	int crc32;
} COMM_PKG_HEADER, *LPCOMM_PKG_HEADER;

#pragma pack()

#endif
