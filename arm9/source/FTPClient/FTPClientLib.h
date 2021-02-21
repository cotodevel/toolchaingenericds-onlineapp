/***************************************************************************/
/*                                                                         */
/* ftplib.h - Part of Open C FTP Client                                    */
/*                                                                         */
/* Copyright (C) 2007 Nokia Corporation                                    */
/*                                                                         */
/* Open C FTP Client is based on ftplib and qftp by Thomas Pfau.           */
/*                                                                         */
/* Copyright (C) 1996-2001 Thomas Pfau, pfau@eclipse.net                   */
/* 1407 Thomas Ave, North Brunswick, NJ, 08902                             */
/*                                                                         */
/* This program is free software; you can redistribute it and/or           */
/* modify it under the terms of the GNU Library General Public             */
/* License as published by the Free Software Foundation; either            */
/* version 2 of the License, or (at your option) any later version.        */
/*                                                                         */
/* This program is distributed in the hope that it will be useful,         */
/* but WITHOUT ANY WARRANTY; without even the implied warranty of          */
/* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU       */
/* Library General Public License for more details.                        */
/*                                                                         */
/* You should have received a copy of the GNU Library General Public       */
/* License along with this program; if not, write to the                   */
/* Free Software Foundation, Inc., 59 Temple Place - Suite 330,            */
/* Boston, MA 02111-1307, USA.                                             */
/*                                                                         */
/***************************************************************************/
 
/*
 * ============================================================================
 *  Name     : ftplib from ftplib.h
 *  Part of  : Open C FTP Client
 *  Created  : 03/12/2007 by Forum Nokia
 *  Version  : 1.0
 * ============================================================================
*/

#ifndef __FTPLIB_H
#define __FTPLIB_H

#define NEED_MEMCCPY

#include <time.h>

/* This original code is commented out due to the conversion changes */
/*
#if !defined(__FTPLIB_H)
#define __FTPLIB_H

//#if defined(__unix__) || defined(VMS)
#define GLOBALDEF
#define GLOBALREF extern

#elif defined(_WIN32)
#if defined BUILDING_LIBRARY
#define GLOBALDEF __declspec(dllexport)
#define GLOBALREF __declspec(dllexport)
#else
#define GLOBALREF __declspec(dllimport)
#endif

#endif
*/

/* FtpAccess() type codes */
#define FTPLIB_DIR 1
#define FTPLIB_DIR_VERBOSE 2
#define FTPLIB_FILE_READ 3
#define FTPLIB_FILE_WRITE 4

/* FtpAccess() mode codes */
#define FTPLIB_ASCII 'A'
#define FTPLIB_IMAGE 'I'
#define FTPLIB_TEXT FTPLIB_ASCII
#define FTPLIB_BINARY FTPLIB_IMAGE

/* connection modes */
#define FTPLIB_PASSIVE 1
#define FTPLIB_PORT 2

/* connection option names */
#define FTPLIB_CONNMODE 1
#define FTPLIB_CALLBACK 2
#define FTPLIB_IDLETIME 3
#define FTPLIB_CALLBACKARG 4
#define FTPLIB_CALLBACKBYTES 5

#ifdef __cplusplus
extern "C" {
#endif




static char *rvs = "0";
static int rvsused = 0;
static char *rvstmp;
static char *dbuf;

extern void *memccpy(void *dest, const void *src, int c, size_t n);

/* v1 compatibility stuff */

/* This original code is commented out due to the conversion changes */
/*
#if !defined(_FTPLIB_NO_COMPAT)
struct NetBuf *DefaultNetbuf;

#define ftplib_lastresp FtpLastResponse(DefaultNetbuf)
#define ftpInit FtpInit
#define ftpOpen(x) FtpConnect(x, &DefaultNetbuf)
#define ftpLogin(x,y) FtpLogin(x, y, DefaultNetbuf)
#define ftpSite(x) FtpSite(x, DefaultNetbuf)
#define ftpMkdir(x) FtpMkdir(x, DefaultNetbuf)
#define ftpChdir(x) FtpChdir(x, DefaultNetbuf)
#define ftpRmdir(x) FtpRmdir(x, DefaultNetbuf)
#define ftpNlst(x, y) FtpNlst(x, y, DefaultNetbuf)
#define ftpDir(x, y) FtpDir(x, y, DefaultNetbuf)
#define ftpGet(x, y, z) FtpGet(x, y, z, DefaultNetbuf)
#define ftpPut(x, y, z) FtpPut(x, y, z, DefaultNetbuf)
#define ftpRename(x, y) FtpRename(x, y, DefaultNetbuf)
#define ftpDelete(x) FtpDelete(x, DefaultNetbuf)
#define ftpQuit() FtpQuit(DefaultNetbuf)
#endif */ /* (_FTPLIB_NO_COMPAT) */
/* end v1 compatibility stuff */

/*
GLOBALREF int ftplib_debug;
GLOBALREF void FtpInit(void);
GLOBALREF char *FtpLastResponse(struct NetBuf *nControl);
GLOBALREF int FtpConnect(const char *host, struct NetBuf **nControl);
GLOBALREF int FtpOptions(int opt, long val, struct NetBuf *nControl);
GLOBALREF int FtpLogin(const char *user, const char *pass, struct NetBuf *nControl);
GLOBALREF int FtpAccess(const char *path, int typ, int mode, struct NetBuf *nControl,
    struct NetBuf **nData);
GLOBALREF int FtpRead(void *buf, int max, struct NetBuf *nData);
GLOBALREF int FtpWrite(void *buf, int len, struct NetBuf *nData);
GLOBALREF int FtpClose(struct NetBuf *nData);
GLOBALREF int FtpSite(const char *cmd, struct NetBuf *nControl);
GLOBALREF int FtpSysType(char *buf, int max, struct NetBuf *nControl);
GLOBALREF int FtpMkdir(const char *path, struct NetBuf *nControl);
GLOBALREF int FtpChdir(const char *path, struct NetBuf *nControl);
GLOBALREF int FtpCDUp(struct NetBuf *nControl);
GLOBALREF int FtpRmdir(const char *path, struct NetBuf *nControl);
GLOBALREF int FtpPwd(char *path, int max, struct NetBuf *nControl);
GLOBALREF int FtpNlst(const char *output, const char *path, struct NetBuf *nControl);
GLOBALREF int FtpDir(const char *output, const char *path, struct NetBuf *nControl);
GLOBALREF int FtpSize(const char *path, int *size, char mode, struct NetBuf *nControl);
GLOBALREF int FtpModDate(const char *path, char *dt, int max, struct NetBuf *nControl);
GLOBALREF int FtpGet(const char *output, const char *path, char mode,
	struct NetBuf *nControl);
GLOBALREF int FtpPut(const char *input, const char *path, char mode,
	struct NetBuf *nControl);
GLOBALREF int FtpRename(const char *src, const char *dst, struct NetBuf *nControl);
GLOBALREF int FtpDelete(const char *fnm, struct NetBuf *nControl);
GLOBALREF void FtpQuit(struct NetBuf *nControl);
*/

/* This original code is commented out due to the conversion changes */
//int ftplib_debug;
char *FtpLastResponse(struct NetBuf *nControl);
int FtpConnect(const char *host, struct NetBuf **nControl);
int FtpOptions(int opt, long val, struct NetBuf *nControl);
int FtpLogin(const char *user, const char *pass, struct NetBuf *nControl);
int FtpAccess(const char *path, int typ, int mode, struct NetBuf *nControl, struct NetBuf **nData);
int FtpRead(void *buf, int max, struct NetBuf *nData);
int FtpWrite(void *buf, int len, struct NetBuf *nData);
int FtpClose(struct NetBuf *nData);
int FtpSite(const char *cmd, struct NetBuf *nControl);
int FtpSysType(char *buf, int max, struct NetBuf *nControl);
int FtpMkdir(const char *path, struct NetBuf *nControl);
int FtpChdir(const char *path, struct NetBuf *nControl);
int FtpCDUp(struct NetBuf *nControl);
int FtpRmdir(const char *path, struct NetBuf *nControl);
int FtpPwd(char *path, int max, struct NetBuf *nControl);
char* FtpNlst(const char *output, const char *path, struct NetBuf *nControl);
char* FtpDir(const char *output, const char *path, struct NetBuf *nControl);
int FtpSize(const char *path, int *size, char mode, struct NetBuf *nControl);
int FtpModDate(const char *path, char *dt, int max, struct NetBuf *nControl);
char* FtpGet(const char *output, const char *path, char mode,	struct NetBuf *nControl);
int FtpPut(const char *input, const char *path, char mode,	struct NetBuf *nControl);
int FtpRename(const char *src, const char *dst, struct NetBuf *nControl);
char* FtpDelete(const char *fnm, struct NetBuf *nControl);
void FtpQuit(struct NetBuf *nControl);

#ifdef __cplusplus
}
#endif

#endif /* __FTPLIB_H */
