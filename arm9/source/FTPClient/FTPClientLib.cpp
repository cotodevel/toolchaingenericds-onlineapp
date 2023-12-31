/***************************************************************************/
/*                                                                         */
/* ftplib.c - Part of Open C FTP Client                                    */
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
 *  Name     : FTPLIB from ftplib.c
 *  Part of  : Open C FTP Client
 *  Created  : 03/12/2007 by Forum Nokia
 *  Version  : 1.0
 * ============================================================================
*/
#include "typedefsTGDS.h"
#include "posixHandleTGDS.h"
#include "dsregs.h"
#include "dsregs_asm.h"
//#include "gui.h"
//#include "guiGlobal.h"
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
/* This original code is commented out due to the conversion changes */
//#if defined(__unix__)
#include <sys/time.h>
//#include <sys/select.h>
#include <sys/types.h>
//#include <sys/socket.h>
//#include <netinet/in.h>
//#include <netdb.h>
#include <socket.h>
#include <in.h>
#include <netdb.h>
#include <dswifi9.h>
#include "WoopsiTemplate.h"

//#include <arpa/inet.h>
/* This original code is commented out due to the conversion changes */
/*
#elif defined(VMS)
#include <types.h>
#include <socket.h>
#include <in.h>
#include <netdb.h>
#include <inet.h>
#elif defined(_WIN32)
#include <winsock.h>
#endif
*/

#include "WoopsiTemplate.h"

//Progress bar wrapper function for calling C++ functionality
void callSetProgressWrapper(int value);

#define BUILDING_LIBRARY
#include "FTPClientLib.h"

/* This original code is commented out due to the conversion changes */
//#if defined(_WIN32)
//#define SETSOCKOPT_OPTVAL_TYPE (const char *)
//#else
#define SETSOCKOPT_OPTVAL_TYPE (void *)
/* This original code is commented out due to the conversion changes */
//#endif

#define FTPLIB_BUFSIZ 8191
//#define FTPLIB_BUFSIZ 400
#define ACCEPT_TIMEOUT 30

#define FTPLIB_CONTROL 0
#define FTPLIB_READ 1
#define FTPLIB_WRITE 2

#if !defined FTPLIB_DEFMODE
#define FTPLIB_DEFMODE FTPLIB_PASSIVE
#endif


/* This original code is commented out due to the conversion changes */
//static char *version = "ftplib Release 3.1-1 9/16/00, copyright 1996-2000 Thomas Pfau";
//GLOBALDEF int ftplib_debug = 0;
int ftplib_debug = 0;

/* This original code is commented out due to the conversion changes */
//#if defined(__unix__) || defined(VMS)
//#define net_read read
//#define net_write write
//#define net_close close
/* This original code is commented out due to the conversion changes */
/*
#elif defined(_WIN32)
#define net_read(x,y,z) recv(x,y,z,0)
#define net_write(x,y,z) send(x,y,z,0)
#define net_close closesocket
#endif
*/

#define net_read(x,y,z) recv(x,y,z,0)
#define net_close closesocket
#define net_write(x,y,z) send(x,y,z,0)

#if defined(NEED_MEMCCPY)
/*
 * VAX C does not supply a memccpy routine so I provide my own
 */

#if (defined(__GNUC__) && !defined(__clang__))
__attribute__((optimize("O0")))
#endif

#if (!defined(__GNUC__) && defined(__clang__))
__attribute__ ((optnone))
#endif
void *memccpy(void *dest, const void *src, int c, size_t n) 
{
    int i=0;
    const unsigned char *ip=(const unsigned char *)src;
    unsigned char *op=(unsigned char *)dest;

    while (i < n)
    {
	if ((*op++ = *ip++) == c)
	    break;
	i++;
    }
    if (i == n)
	return NULL;
    return op;
}
#endif
#if defined(NEED_STRDUP)
/*
 * strdup - return a malloc'ed copy of a string
 */
char *strdup(const char *src)
{
    int l = strlen(src) + 1;
    char *dst = TGDSARM9Malloc(l);
    if (dst)
        strcpy(dst,src);
    return dst;
}
#endif

/*
 * socket_wait - wait for socket to receive or flush data
 *
 * return 1 if no user callback, otherwise, return value returned by
 * user callback
 */

#if (defined(__GNUC__) && !defined(__clang__))
__attribute__((optimize("O0")))
#endif

#if (!defined(__GNUC__) && defined(__clang__))
__attribute__ ((optnone))
#endif
static int socket_wait(struct NetBuf *ctl) 
{
    fd_set fd,*rfd = NULL,*wfd = NULL;
    struct timeval tv;
    int rv = 0;

    if ((ctl->dir == FTPLIB_CONTROL) || (ctl->idlecb == NULL))
		return 1;
    if (ctl->dir == FTPLIB_WRITE)
		wfd = &fd;
    else
		rfd = &fd;

    FD_ZERO(&fd);

	do {
		FD_SET(ctl->handle,&fd);
		tv = ctl->idletime;
		rv = select(ctl->handle+1, rfd, wfd, NULL, &tv);
		if (rv == -1) {
			rv = 0;
			struct NetBuf * handleCtrl = (struct NetBuf *)ctl->ctrl;
			strncpy(handleCtrl->response, strerror(errno),sizeof(handleCtrl->response));
			break;
		} else if (rv > 0)	{
			rv = 1;
			break;
		}
    } while((rv = ctl->idlecb(ctl, ctl->xfered, ctl->idlearg)));
    return rv;
}

/*
 * read a line of text
 *
 * return -1 on error or bytecount
 */

#if (defined(__GNUC__) && !defined(__clang__))
__attribute__((optimize("O0")))
#endif

#if (!defined(__GNUC__) && defined(__clang__))
__attribute__ ((optnone))
#endif
int readline(char *buf,int max,struct NetBuf *ctl) 
{
    int x,retval = 0;
    char *end,*bp=buf;
    int eof = 0;

    if((ctl->dir != FTPLIB_CONTROL) && (ctl->dir != FTPLIB_READ)) {
		printf("ctl->dir != _CONTROL && != _READ");
		return -1;
    }
    if(max == 0)
		return 0;

    do 
	{
    	if (ctl->cavail > 0) 
		{
			x = (max >= ctl->cavail) ? ctl->cavail : max-1;
			end = (char*)memccpy(bp,ctl->cget,'\n',x);
			if (end != NULL)
				x = end - bp;

			retval += x;
			bp += x;
			*bp = '\0';
			max -= x;
			ctl->cget += x;
			ctl->cavail -= x;
			if(end != NULL) 
			{
				bp -= 2;
				if (strcmp(bp,"\r\n") == 0) 
				{
					*bp++ = '\n';
					*bp++ = '\0';
					--retval;
				}
				break;
			}
    	}
    	if (max == 1)
    	{
			*buf = '\0';
			break;
    	}
    	if (ctl->cput == ctl->cget)
    	{
			ctl->cput = ctl->cget = ctl->buf;
			ctl->cavail = 0;
			ctl->cleft = FTPLIB_BUFSIZ;
    	}
		if (eof)
		{
			if(retval == 0)
				retval = -1;
			break;
		}
		if(!socket_wait(ctl))
		{
			printf("retval set and returning");
			return retval;
		}
    	
		if ((x = net_read(ctl->handle,ctl->cput,ctl->cleft)) == -1)
		{
			// For some reason at the end of file -1 is returned instead of expected 0
			//Seems to be erroring here
			printf("retval set to -1");
			retval = -1;
			break;
   		}
		if(x == 0)
		{
			// We found an eof
			eof = 1;
		}
		ctl->cleft -= x;
	    ctl->cavail += x;
	    ctl->cput += x;
	} while (1);

    return retval;
}

/* write lines of text
 *
 * return -1 on error or bytecount
 */

#if (defined(__GNUC__) && !defined(__clang__))
__attribute__((optimize("O0")))
#endif

#if (!defined(__GNUC__) && defined(__clang__))
__attribute__ ((optnone))
#endif
static int writeline(char *buf, int len, struct NetBuf *nData) 
{
    int x, nb=0, w;
    char *ubp = buf, *nbp;
    char lc=0;

    if(nData->dir != FTPLIB_WRITE)
		return -1;
    nbp = nData->buf;
    for (x=0; x < len; x++)
    {
		if ((*ubp == '\n') && (lc != '\r'))
		{
			if (nb == FTPLIB_BUFSIZ)
			{
				if (!socket_wait(nData))
					return x;
				w = net_write(nData->handle, nbp, FTPLIB_BUFSIZ);
				if (w != FTPLIB_BUFSIZ)
				{
					return(-1);
				}
				nb = 0;
			}
			nbp[nb++] = '\r';
		}
		if (nb == FTPLIB_BUFSIZ)
		{
			if (!socket_wait(nData))
				return x;
			w = net_write(nData->handle, nbp, FTPLIB_BUFSIZ);
			if (w != FTPLIB_BUFSIZ)
			{
				printf("net_write(2) returned %d", w);
				return(-1);
			}
			nb = 0;
		}
		nbp[nb++] = lc = *ubp++;
    }
    if (nb)
    {
		if (!socket_wait(nData))
			return x;
		w = net_write(nData->handle, nbp, nb);
		if (w != nb)
		{
			printf("net_write(3) returned %d", w);
			return(-1);
		}
    }
    return len;
}

/*
 * read a response from the server
 *
 * return 0 if first char doesn't match
 * return 1 if first char matches
 */

#if (defined(__GNUC__) && !defined(__clang__))
__attribute__((optimize("O0")))
#endif

#if (!defined(__GNUC__) && defined(__clang__))
__attribute__ ((optnone))
#endif
static int readresp(char c, struct NetBuf *nControl) 
{
    char match[5];
    if (readline(nControl->response,256,nControl) == -1)
    {	
		//this is where it fails
		printf("readline#1 returned -1");
		return 0;
    }
	if (nControl->response[3] == '-')
    {
		strncpy(match,nControl->response,3);
		match[3] = ' ';
		match[4] = '\0';
		do
		{
			if (readline(nControl->response,256,nControl) == -1)
			{
				printf("readline#2 returned -1");
				return 0;
			}
		} while(strncmp(nControl->response,match,4));
    }
    if(nControl->response[0] == c)
    {
		return 1;
    }
	printf("failed to match some things that should have been matched");
	printf("response: %s", nControl->response);
    return 0;
}


/*
 * FtpLastResponse - return a pointer to the last response received
 */
char *FtpLastResponse(struct NetBuf *nControl)
{
    if ((nControl) && (nControl->dir == FTPLIB_CONTROL))
    	return nControl->response;
    return NULL;
}

/*
 * FtpConnect - connect to remote server
 *
 * return 1 if connected, 0 if not
 */
/* This original code is commented out due to the conversion changes */
//GLOBALDEF int FtpConnect(const char *host, struct NetBuf **nControl)

#if (defined(__GNUC__) && !defined(__clang__))
__attribute__((optimize("O0")))
#endif

#if (!defined(__GNUC__) && defined(__clang__))
__attribute__ ((optnone))
#endif
int FtpConnect(const char *host, struct NetBuf **nControl) 
{
    int sControl;
    struct sockaddr_in sin;
    struct hostent *phe;
    //struct servent *pse;
    int on=1;
    struct NetBuf *ctrl;
    //char *lhost;
    char *pnum;

    memset(&sin,0,sizeof(sin));
    sin.sin_family = AF_INET;
    //lhost = strdup(host);
    //pnum = strchr(lhost,':');
//    if (pnum == NULL)
//    {
/* This original code is commented out due to the conversion changes */
/*
#if defined(VMS)
    	sin.sin_port = htons(21);
#else
*/
//    	if ((pse = getservbyname("ftp","tcp")) == NULL)
//    	{
/* This original code is commented out due to the conversion changes */
//	    perror("getservbyname");
	    //return 0;
    	//}
    	//sin.sin_port = pse->s_port;
		//sin.sin_port = 21;
/* This original code is commented out due to the conversion changes */
//#endif
//    }
//    else
//    {
//	*pnum++ = '\0';
//	if (isdigit(*pnum))
//	    sin.sin_port = htons((short)atoi(pnum));
//	else
//	{
	    //pse = getservbyname(pnum,"tcp");
	    //sin.sin_port = pse->s_port;
//		sin.sin_port = 21;
//	}
//    }
//    if ((sin.sin_addr.s_addr = inet_addr(lhost)) == -1)
//    {
//    	if ((phe = gethostbyname(lhost)) == NULL)
//    	{
/* This original code is commented out due to the conversion changes */
//	    perror("gethostbyname");
	    //return 0;
    //	}
		//printf("connected to %s",phe->h_addr_list[0]);
		phe = gethostbyname( host );
		if(phe) printf("Got Host");
		sControl = socket(PF_INET, SOCK_STREAM, 0);
		
		sin.sin_family = AF_INET;
		sin.sin_port = htons(21);
		sin.sin_addr.s_addr = *( (unsigned long *)(phe->h_addr_list[0]) );
    	// memcpy((char *)&sin.sin_addr, phe->h_addr, phe->h_length);
    //TGDSARM9Free(lhost);
    
    if (sControl == -1)
    {
		printf("Failed to create socket");
		return 0;
    }
    if (setsockopt(sControl,SOL_SOCKET,SO_REUSEADDR,
		   SETSOCKOPT_OPTVAL_TYPE &on, sizeof(on)) == -1)
    {
		printf("Failed to set socket option");
		net_close(sControl);
		return 0;
    }
    if(connect(sControl, (struct sockaddr *)&sin, sizeof(sin)) == -1)
    {
		printf("Failed to connect");
		net_close(sControl);
		return 0;
    }
	printf("We should be connected");
    ctrl = (struct NetBuf *)TGDSARM9Calloc(1,sizeof(struct NetBuf));
    if (ctrl == NULL)
    {
		net_close(sControl);
		return 0;
    }
    ctrl->buf = (char*)TGDSARM9Malloc(FTPLIB_BUFSIZ);
    if (ctrl->buf == NULL)
    {
		net_close(sControl);
		TGDSARM9Free(ctrl);
		return 0;
    }
	printf("Getting ready to set options on ctrl");
    ctrl->handle = sControl;
    ctrl->dir = FTPLIB_CONTROL;
    ctrl->ctrl = NULL;
    ctrl->cmode = FTPLIB_DEFMODE;
    ctrl->idlecb = NULL;
    ctrl->idletime.tv_sec = ctrl->idletime.tv_usec = 0;
    ctrl->idlearg = NULL;
    ctrl->xfered = 0;
    ctrl->xfered1 = 0;
    ctrl->cbbytes = 0;
    if (readresp('2', ctrl) == 0)
    {
		printf("read response returned null, apparently that means fail?");
		net_close(sControl);
		TGDSARM9Free(ctrl->buf);
		TGDSARM9Free(ctrl);
		return 0;
    }
    *nControl = ctrl;
    return 1;
}

/*
 * FtpOptions - change connection options
 *
 * returns 1 if successful, 0 on error
 */

#if (defined(__GNUC__) && !defined(__clang__))
__attribute__((optimize("O0")))
#endif

#if (!defined(__GNUC__) && defined(__clang__))
__attribute__ ((optnone))
#endif
int FtpOptions(int opt, long val, struct NetBuf *nControl) 
{
    int v,rv=0;
    switch (opt)
    {
      case FTPLIB_CONNMODE:
		v = (int) val;
		if ((v == FTPLIB_PASSIVE) || (v == FTPLIB_PORT))
		{
			nControl->cmode = v;
			rv = 1;
		}
		break;
      case FTPLIB_CALLBACK:
		nControl->idlecb = (FtpCallback) val;
		rv = 1;
		break;
      case FTPLIB_IDLETIME:
		v = (int) val;
		rv = 1;
		nControl->idletime.tv_sec = v / 1000;
		nControl->idletime.tv_usec = (v % 1000) * 1000;
		break;
      case FTPLIB_CALLBACKARG:
		rv = 1;
		nControl->idlearg = (void *) val;
		break;
      case FTPLIB_CALLBACKBYTES:
        rv = 1;
        nControl->cbbytes = (int) val;
        break;
    }
    return rv;
}

/*
 * FtpSendCmd - send a command and wait for expected response
 *
 * return 1 if proper response received, 0 otherwise
 */

#if (defined(__GNUC__) && !defined(__clang__))
__attribute__((optimize("O0")))
#endif

#if (!defined(__GNUC__) && defined(__clang__))
__attribute__ ((optnone))
#endif
static int FtpSendCmd(const char *cmd, char expresp, struct NetBuf *nControl) 
{
    char buf[256];
    if(nControl->dir != FTPLIB_CONTROL)
		return 0;
    //if(ftplib_debug > 2)
	//	fprintf(stderr,"%s\n",cmd);
    if((strlen(cmd) + 3) > sizeof(buf))
        return 0;

    sprintf(buf,"%s\r\n",cmd);
    if(net_write(nControl->handle,buf,strlen(buf)) <= 0)
    {
		printf("write error in FtpSendCmd");
		return 0;
    }
    return readresp(expresp, nControl);
}

/*
 * FtpLogin - log in to remote server
 *
 * return 1 if logged in, 0 otherwise
 */
/* This original code is commented out due to the conversion changes */
//GLOBALDEF int FtpLogin(const char *user, const char *pass, struct NetBuf *nControl)

#if (defined(__GNUC__) && !defined(__clang__))
__attribute__((optimize("O0")))
#endif

#if (!defined(__GNUC__) && defined(__clang__))
__attribute__ ((optnone))
#endif
int FtpLogin(const char *user, const char *pass, struct NetBuf *nControl) 
{
    char tempbuf[64];

    if (((strlen(user) + 7) > sizeof(tempbuf)) ||
        ((strlen(pass) + 7) > sizeof(tempbuf)))
        return 0;

    sprintf(tempbuf,"USER %s",user);
    if(!FtpSendCmd(tempbuf,'3',nControl)) 
	{
		if(nControl->response[0] == '2')
		    return 1;
		return 0;
    }
    sprintf(tempbuf,"PASS %s",pass);
    return FtpSendCmd(tempbuf,'2',nControl);
}

/*
 * FtpOpenPort - set up data connection
 *
 * return 1 if successful, 0 otherwise
 */

#if (defined(__GNUC__) && !defined(__clang__))
__attribute__((optimize("O0")))
#endif

#if (!defined(__GNUC__) && defined(__clang__))
__attribute__ ((optnone))
#endif
static int FtpOpenPort(struct NetBuf *nControl, struct NetBuf **nData, int mode, int dir) 
{
    int sData;

	struct sockaddr_in in;
	char ip_addr[30];
	char *ip_tmp;
	int i;
	unsigned int port = 0, portOctet1 = 0, portOctet2 = 0;

/* This original code is commented out due to the conversion changes */
/*
    union {
	struct sockaddr sa;
	struct sockaddr_in in;
    } sin;
    struct linger lng = { 0, 0 };
*/  int on=1;
    struct NetBuf *ctrl;
    char *cp;
/* This original code is commented out due to the conversion changes */
//    unsigned int v[6];
//    char buf[256];

    if (nControl->dir != FTPLIB_CONTROL)
		return -1;
    if ((dir != FTPLIB_READ) && (dir != FTPLIB_WRITE))
    {
		sprintf(nControl->response, "Invalid direction %d\n", dir);
		return -1;
    }
    if ((mode != FTPLIB_ASCII) && (mode != FTPLIB_IMAGE))
    {
		sprintf(nControl->response, "Invalid mode %c\n", mode);
		return -1;
    }

/* This original code is commented out due to the conversion changes */
/*
    l = sizeof(sa);
    l = sizeof(sin);    
    if (nControl->cmode == FTPLIB_PASSIVE)
    {
	memset(&sa, 0, l);
	memset(&sin, 0, l);	
	in.sin_family = AF_INET;
	sin.in.sin_family = AF_INET;
*/
	if (!FtpSendCmd("PASV",'2',nControl))
	    return -1;
	cp = strchr(nControl->response,'(');
	cp++;
	if (cp == NULL)
	    return -1;

//  Process address and port
//  First, get the address
	ip_tmp = strtok(cp, ",");
	strcpy(ip_addr,ip_tmp);
	for(i=0;i<3;i++)
	{
		strcat(ip_addr, ".");
		ip_tmp = strtok(NULL, ",");
		strcat(ip_addr,ip_tmp);
	}

//  Now get the port number
	portOctet1 = atoi(strtok(NULL, ","));
	portOctet2 = atoi(strtok(NULL, ")"));
	port = portOctet1 * 256 + portOctet2;

/* This original code is commented out due to the conversion changes */	    
/*	sscanf(cp,"%u,%u,%u,%u,%u,%u",&v[2],&v[3],&v[4],&v[5],&v[0],&v[1]);

	sa.sa_data[2] = v[2];
	sa.sa_data[3] = v[3];
	sa.sa_data[4] = v[4];
	sa.sa_data[5] = v[5];
	sa.sa_data[0] = v[0];
	sa.sa_data[1] = v[1];
    }
    else
    {
	if (getsockname(nControl->handle, &sin.sa, &l) < 0)	
	{
	    perror("getsockname");
	    return 0;
	}
    }
*/  sData = socket(PF_INET,SOCK_STREAM,0);
    if (sData == -1)
    {
		printf("Socket error");
		return -1;
    }
    if (setsockopt(sData,SOL_SOCKET,SO_REUSEADDR,
		   SETSOCKOPT_OPTVAL_TYPE &on,sizeof(on)) == -1)
    {
		printf("setsockopt");
		net_close(sData);
		return -1;
    }
/* This original code is commented out due to the conversion changes */
//	SO_LINGER cannot be used, not supported
/*  if (setsockopt(sData,SOL_SOCKET,SO_LINGER,
		   SETSOCKOPT_OPTVAL_TYPE &lng,sizeof(lng)) == -1)
    {
	perror("setsockopt");
	net_close(sData);
	getchar();
	return -1;
    }
*/
    in.sin_family = AF_INET;
    in.sin_addr.s_addr = inet_addr(ip_addr);
    in.sin_port = htons(port);
    bzero(&(in.sin_zero), 8);

/* This original code is commented out due to the conversion changes */
/*    if (nControl->cmode == FTPLIB_PASSIVE)
    {
	if (connect(sData, &sa, sizeof(sa)) == -1)
*/
	if (connect(sData, (struct sockaddr*)&in, sizeof(struct sockaddr)) == -1)
	{
/* This original code is commented out due to the conversion changes */
//	    perror("connect");
	    net_close(sData);
	    return -1;
	}
/* This original code is commented out due to the conversion changes */
/*  }
    else
    {
	if (bind(sData, &sin.sa, sizeof(sin)) == -1)
	{
	    perror("bind");
	    net_close(sData);
	    return 0;
	}
	if (listen(sData, 1) < 0)
	{
	    perror("listen");
	    net_close(sData);
	    return 0;
	}
	if (getsockname(sData, &sin.sa, &l) < 0)
	    return 0;
		
	sprintf(buf, "PORT %d,%d,%d,%d,%d,%d",
		(unsigned char) sin.sa.sa_data[2],
		(unsigned char) sin.sa.sa_data[3],
		(unsigned char) sin.sa.sa_data[4],
		(unsigned char) sin.sa.sa_data[5],
		(unsigned char) sin.sa.sa_data[0],
		(unsigned char) sin.sa.sa_data[1]);


	if (!FtpSendCmd(buf,'2',nControl))
	{
	    net_close(sData);
	    return 0;
	}
    }
*/
    ctrl = (struct NetBuf *)TGDSARM9Calloc(1,sizeof(struct NetBuf));
    if (ctrl == NULL)
    {
		net_close(sData);
		return -1;
    }
    if ((mode == 'A') && ((ctrl->buf = (char *)TGDSARM9Malloc(FTPLIB_BUFSIZ)) == NULL))
    {
		net_close(sData);
		TGDSARM9Free(ctrl);
		return -1;
    }
    ctrl->handle = sData;
    ctrl->dir = dir;
    ctrl->idletime = nControl->idletime;
    ctrl->idlearg = nControl->idlearg;
    ctrl->xfered = 0;
    ctrl->xfered1 = 0;
    ctrl->cbbytes = nControl->cbbytes;
    if (ctrl->idletime.tv_sec || ctrl->idletime.tv_usec || ctrl->cbbytes)
		ctrl->idlecb = nControl->idlecb;
    else
		ctrl->idlecb = NULL;
    *nData = ctrl;
    return 1;
}

/*
 * FtpAcceptConnection - accept connection from server
 *
 * return 1 if successful, 0 otherwise
 */

#if (defined(__GNUC__) && !defined(__clang__))
__attribute__((optimize("O0")))
#endif

#if (!defined(__GNUC__) && defined(__clang__))
__attribute__ ((optnone))
#endif
static int FtpAcceptConnection(struct NetBuf *nData, struct NetBuf *nControl) 
{
    int sData;
    struct sockaddr addr;
    unsigned int l;
    int i;
    struct timeval tv;
    fd_set mask;
    int rv = 0;

    FD_ZERO(&mask);
    FD_SET(nControl->handle, &mask);
    FD_SET(nData->handle, &mask);
    tv.tv_usec = 0;
    tv.tv_sec = ACCEPT_TIMEOUT;
    i = nControl->handle;
    if (i < nData->handle)
		i = nData->handle;

    i = select(i+1, &mask, NULL, NULL, &tv);
    if (i == -1)
    {
        strncpy(nControl->response, strerror(errno),sizeof(nControl->response));
        net_close(nData->handle);
        nData->handle = 0;
        rv = 0;
    }
    else if (i == 0)
    {
		strcpy(nControl->response, "timed out waiting for connection");
		net_close(nData->handle);
		nData->handle = 0;
		rv = 0;
    }
    else
    {
		if (FD_ISSET(nData->handle, &mask))
		{
			l = sizeof(addr);
			sData = accept(nData->handle, &addr, (int *)&l);
			i = errno;
			net_close(nData->handle);
			if (sData > 0)
			{
				rv = 1;
				nData->handle = sData;
			}
			else
			{
				strncpy(nControl->response, strerror(i),sizeof(nControl->response));
				nData->handle = 0;
				rv = 0;
			}
		}
		else if (FD_ISSET(nControl->handle, &mask))
		{
			net_close(nData->handle);
			nData->handle = 0;
			readresp('2', nControl);
			rv = 0;
		}
    }
    return rv;	
}

/*
 * FtpAccess - return a handle for a data stream
 *
 * return 1 if successful, 0 otherwise
 */

#if (defined(__GNUC__) && !defined(__clang__))
__attribute__((optimize("O0")))
#endif

#if (!defined(__GNUC__) && defined(__clang__))
__attribute__ ((optnone))
#endif
int FtpAccess(const char *path, int typ, int mode, struct NetBuf *nControl, struct NetBuf **nData) 
{
    char buf[256];
    int dir;
    if ((path == NULL) && ((typ == FTPLIB_FILE_WRITE) || (typ == FTPLIB_FILE_READ)))
    {
		sprintf(nControl->response,"Missing path argument for file transfer\n");
		return 0;
    }
    sprintf(buf, "TYPE %c", mode);
    if(!FtpSendCmd(buf, '2', nControl))
		return 0;

    switch (typ)
    {
      case FTPLIB_DIR:
		strcpy(buf,"NLST");
		dir = FTPLIB_READ;
		// Change path to NULL
		path = NULL;
		break;
      case FTPLIB_DIR_VERBOSE:
		strcpy(buf,"LIST");
		dir = FTPLIB_READ;
		// Change path to NULL
		path = NULL;	
		break;
      case FTPLIB_FILE_READ:
		strcpy(buf,"RETR");
		dir = FTPLIB_READ;
		break;
      case FTPLIB_FILE_WRITE:
		strcpy(buf,"STOR");
		dir = FTPLIB_WRITE;
		break;
      default:
		sprintf(nControl->response, "Invalid open type %d\n", typ);
		return 0;
    }
    if(path != NULL)
    {
        int i = strlen(buf);
        buf[i++] = ' ';
        if ((strlen(path) + i) >= sizeof(buf))
            return 0;
        strcpy(&buf[i],path);
    }
    if(FtpOpenPort(nControl, nData, mode, dir) == -1)
		return 0;

    if(!FtpSendCmd(buf, '1', nControl))
    {
		printf("FtpSendCmd error, closing nData...");
		FtpClose(*nData);
		*nData = NULL;
		return 0;
    }
    (*nData)->ctrl = nControl;
    nControl->data = *nData;
    if (nControl->cmode == FTPLIB_PORT)
    {
		if (!FtpAcceptConnection(*nData,nControl))
		{
		    FtpClose(*nData);
		    *nData = NULL;
		    nControl->data = NULL;
		    return 0;
		}
    }
    return 1;
}

/*
 * FtpRead - read from a data connection
 */

#if (defined(__GNUC__) && !defined(__clang__))
__attribute__((optimize("O0")))
#endif

#if (!defined(__GNUC__) && defined(__clang__))
__attribute__ ((optnone))
#endif
int FtpRead(void *buf, int max, struct NetBuf *nData) 
{
    int i;
    if (nData->dir != FTPLIB_READ)
    {
		return 0;
    }
    if (nData->buf)
    {
        i = readline((char *)buf, max, nData);
    }
    else
    {
        i = socket_wait(nData);
		if (i != 1)
		{
		    return 0;
		}
        i = net_read(nData->handle, buf, max);
    }
    if (i == -1)
    {
		return 0;
    }
    nData->xfered += i;
    if (nData->idlecb && nData->cbbytes)
    {
        nData->xfered1 += i;
        if (nData->xfered1 > nData->cbbytes)
        {
			if (nData->idlecb(nData, nData->xfered, nData->idlearg) == 0)
			{
				return 0;
			}
            nData->xfered1 = 0;
        }
    }
    return i;
}

/*
 * FtpWrite - write to a data connection
 */

#if (defined(__GNUC__) && !defined(__clang__))
__attribute__((optimize("O0")))
#endif

#if (!defined(__GNUC__) && defined(__clang__))
__attribute__ ((optnone))
#endif
int FtpWrite(void *buf, int len, struct NetBuf *nData) 
{
	
   int i;
    if (nData->dir != FTPLIB_WRITE)
	return 0;
    if (nData->buf)
	{
		printf("ndata->buf == true");
    	i = writeline((char *)buf, len, nData);
	}
    else
    {
		printf("ndata->buf == false");
		printf("socket wait returned: %i", socket_wait(nData));
        i = net_write(nData->handle, buf, len);
    }
    nData->xfered += i;
	printf("After net_write i= %i", i);
    if (nData->idlecb && nData->cbbytes)
    {
        nData->xfered1 += i;
        if (nData->xfered1 > nData->cbbytes)
        {
            //OLD :nData->idlecb(nData, nData->xfered, nData->idlearg);
            //kulak 
			if( ! nData->idlecb(nData, nData->xfered, nData->idlearg) ) return 0;
            nData->xfered1 = 0;
        }
    }
    return i;

}

/*
 * FtpClose - close a data connection
 */

#if (defined(__GNUC__) && !defined(__clang__))
__attribute__((optimize("O0")))
#endif

#if (!defined(__GNUC__) && defined(__clang__))
__attribute__ ((optnone))
#endif
int FtpClose(struct NetBuf *nData) 
{
    struct NetBuf *ctrl;
    switch (nData->dir)
    {
      case FTPLIB_WRITE:
		/* potential problem - if buffer flush fails, how to notify user? */
		if (nData->buf != NULL)
			writeline(NULL, 0, nData);
	  case FTPLIB_READ:
		if (nData->buf)
			TGDSARM9Free(nData->buf);
		shutdown(nData->handle,2);
		net_close(nData->handle);
		ctrl = (struct NetBuf *)nData->ctrl;
		TGDSARM9Free(nData);
		if (ctrl)
		{
		    ctrl->data = NULL;
		    return(readresp('2', ctrl));
		}
		return 1;
      case FTPLIB_CONTROL:
		if (nData->data)
		{
		    nData->ctrl = NULL;
		    FtpClose(nData);
		}
		net_close(nData->handle);
		TGDSARM9Free(nData);
		return 0;
	}
    return 1;
}

/*
 * FtpSite - send a SITE command
 *
 * return 1 if command successful, 0 otherwise
 */

#if (defined(__GNUC__) && !defined(__clang__))
__attribute__((optimize("O0")))
#endif

#if (!defined(__GNUC__) && defined(__clang__))
__attribute__ ((optnone))
#endif
int FtpSite(const char *cmd, struct NetBuf *nControl) 
{
    char buf[256];
    
    if ((strlen(cmd) + 7) > sizeof(buf))
        return 0;

    sprintf(buf,"SITE %s",cmd);
    if(!FtpSendCmd(buf,'2',nControl))
		return 0;
    return 1;
}

/*
 * FtpSysType - send a SYST command
 *
 * Fills in the user buffer with the remote system type.  If more
 * information from the response is required, the user can parse
 * it out of the response buffer returned by FtpLastResponse().
 *
 * return 1 if command successful, 0 otherwise
 */

#if (defined(__GNUC__) && !defined(__clang__))
__attribute__((optimize("O0")))
#endif

#if (!defined(__GNUC__) && defined(__clang__))
__attribute__ ((optnone))
#endif
int FtpSysType(char *buf, int max, struct NetBuf *nControl) 
{
    int l = max;
    char *b = buf;
    char *s;
    if(!FtpSendCmd("SYST",'2',nControl))
		return 0;

    s = &nControl->response[4];
    while((--l) && (*s != ' '))
		*b++ = *s++;

    *b++ = '\0';
    return 1;
}

/*
 * FtpMkdir - create a directory at server
 *
 * return 1 if successful, 0 otherwise
 */

#if (defined(__GNUC__) && !defined(__clang__))
__attribute__((optimize("O0")))
#endif

#if (!defined(__GNUC__) && defined(__clang__))
__attribute__ ((optnone))
#endif
int FtpMkdir(const char *path, struct NetBuf *nControl) 
{
    char buf[256];

    if ((strlen(path) + 6) > sizeof(buf))
        return 0;
    sprintf(buf,"MKD %s",path);
    if (!FtpSendCmd(buf,'2', nControl))
		return 0;

    return 1;
}

/*
 * FtpChdir - change path at remote
 *
 * return 1 if successful, 0 otherwise
 */

#if (defined(__GNUC__) && !defined(__clang__))
__attribute__((optimize("O0")))
#endif

#if (!defined(__GNUC__) && defined(__clang__))
__attribute__ ((optnone))
#endif
int FtpChdir(const char *path, struct NetBuf *nControl) 
{
    char buf[256];

    if ((strlen(path) + 6) > sizeof(buf))
        return 0;
    sprintf(buf,"CWD %s",path);
    if (!FtpSendCmd(buf,'2',nControl))
		return 0;

    return 1;
}

/*
 * FtpCDUp - move to parent directory at remote
 *
 * return 1 if successful, 0 otherwise
 */

#if (defined(__GNUC__) && !defined(__clang__))
__attribute__((optimize("O0")))
#endif

#if (!defined(__GNUC__) && defined(__clang__))
__attribute__ ((optnone))
#endif
int FtpCDUp(struct NetBuf *nControl) 
{
    if (!FtpSendCmd("CDUP",'2',nControl))
		return 0;
    return 1;
}

/*
 * FtpRmdir - remove directory at remote
 *
 * return 1 if successful, 0 otherwise
 */
#if (defined(__GNUC__) && !defined(__clang__))
__attribute__((optimize("O0")))
#endif

#if (!defined(__GNUC__) && defined(__clang__))
__attribute__ ((optnone))
#endif
int FtpRmdir(const char *path, struct NetBuf *nControl) 
{
    char buf[256];

    if((strlen(path) + 6) > sizeof(buf))
        return 0;
    sprintf(buf,"RMD %s",path);

    if(!FtpSendCmd(buf,'2',nControl))
		return 0;

    return 1;
}

/*
 * FtpPwd - get working directory at remote
 *
 * return 1 if successful, 0 otherwise
 */

#if (defined(__GNUC__) && !defined(__clang__))
__attribute__((optimize("O0")))
#endif

#if (!defined(__GNUC__) && defined(__clang__))
__attribute__ ((optnone))
#endif
int FtpPwd(char *path, int max, struct NetBuf *nControl) 
{
    int l = max;
    char *b = path;
    char *s;

    if(!FtpSendCmd("PWD",'2',nControl)){
		WoopsiTemplateProc->_MultiLineTextBoxLogger->appendText(WoopsiString("FtpPwd:Failure sending CMD\n"));
		return 0;
	}
    s = strchr(nControl->response, '"');
    if (s == NULL){
		WoopsiTemplateProc->_MultiLineTextBoxLogger->appendText(WoopsiString("FtpPwd:NULL RESPONSE\n"));
		return 0;
    }
	s++;
    while((--l) && (*s) && (*s != '"'))
		*b++ = *s++;

    *b++ = '\0';
    return 1;
}

/*
 * FtpXfer - issue a command and transfer data
 *
 * return 1 if successful, 0 otherwise
 */
#if (defined(__GNUC__) && !defined(__clang__))
__attribute__((optimize("O0")))
#endif

#if (!defined(__GNUC__) && defined(__clang__))
__attribute__ ((optnone))
#endif
static int FtpXfer(const char *localfile, const char *path,struct NetBuf *nControl, int typ, int mode) 
{
	WoopsiTemplateProc->_MultiLineTextBoxLogger->removeText(0);
	WoopsiTemplateProc->_MultiLineTextBoxLogger->moveCursorToPosition(0);
	WoopsiTemplateProc->_MultiLineTextBoxLogger->appendText(WoopsiString(localfile));
	WoopsiTemplateProc->_MultiLineTextBoxLogger->appendText("\n");
	
    int l;
    FILE *local = NULL;
    struct NetBuf *nData;
    int rv=1;

	//rvs--;
    if (localfile != NULL)
    {
		char ac[4] = "w";
		if (typ == FTPLIB_FILE_WRITE)
			ac[0] = 'r';
	
		if (mode == FTPLIB_IMAGE)
			ac[1] = 'b';
		
		char fileBuf[256+1];
		memset(fileBuf, 0, sizeof(fileBuf));
		strcpy(fileBuf, "0:");
		strcat(fileBuf, localfile);
		localfile = &fileBuf[0];
		local = fopen(localfile, "w+");	//treat always as new file and overwrite if exists
		if (local == NULL) {
			//strncpy(nControl->response, strerror(errno),sizeof(nControl->response));
			char arrBuild[256+1];
			sprintf(arrBuild, "failed to open file:%s\n", localfile);
			WoopsiTemplateProc->_MultiLineTextBoxLogger->appendText(WoopsiString(arrBuild));
			return 0;
		}
    }
    if (local == NULL)
		local = (typ == FTPLIB_FILE_WRITE) ? stdin : stdout;

    if(!FtpAccess(path, typ, mode, nControl, &nData))
	{
		//printf("FtpAccess failed...");
		char arrBuild[256+1];
		sprintf(arrBuild, "FtpAccess failed...\n");
		WoopsiTemplateProc->_MultiLineTextBoxLogger->appendText(WoopsiString(arrBuild));
		
		return 0;
	}
    
	dbuf = (char *)TGDSARM9Calloc(1,FTPLIB_BUFSIZ + 1);
    if (typ == FTPLIB_FILE_WRITE)
    {
		while ((l = fread(dbuf, 1, FTPLIB_BUFSIZ, local)) > 0){
			//printf("attempting to write: %i",l);
			if (FtpWrite(dbuf, l, nData) < l)
			{
			    //Code to resend unsent bytes here 
				break;
			}
		}
	}
    else
    {
		WoopsiTemplateProc->_MultiLineTextBoxLogger->appendText("Download start.\n");
		char logConsole[256+1];
    	int total = 0;
		if(rvsused){ TGDSARM9Free(rvs); }
		rvs = (char*)TGDSARM9Calloc(1,FTPLIB_BUFSIZ + 1);
		rvsused = 1;
		while ((l = FtpRead(dbuf, FTPLIB_BUFSIZ, nData)) > 0)
    	{
			total += l;
			//For progress bar... needs to be fixed
			if( local != stdout )
			{
				//callSetProgressWrapper(total);	//todo
			}

			if(local == stdout){
				if( total > FTPLIB_BUFSIZ )
				{
					rvs = (char*)TGDSARM9Realloc(rvs, total*2 + 1);
				}
				strcat(rvs,dbuf);
				//printf("From ftpxfer: %s",rvs);
				//break;			
			}
			else if (fwrite(dbuf, 1, l, local) <= 0)
			{
				//printf("localfile write error");
				rv = 0;
				break;
			}
			//printf("total is %i",total);
			//TGDSARM9Free(rvstmp);
			//TGDSARM9Free(rvs);		

			memset(logConsole, 0, sizeof(logConsole));
			sprintf(logConsole, "Received byte size = %d Total length = %d \n", l, total);
			WoopsiTemplateProc->_MultiLineTextBoxLogger->setText(WoopsiString(logConsole));
    	}
    }
    TGDSARM9Free(dbuf);
    //TGDSARM9Free(rvstmp);
    fflush(local);
    if(localfile != NULL)
		fclose(local);

    FtpClose(nData);

    return rv;
}

/*
 * FtpNlst - issue an NLST command and write response to output
 *
 * return 1 if successful, 0 otherwise
 */

#if (defined(__GNUC__) && !defined(__clang__))
__attribute__((optimize("O0")))
#endif

#if (!defined(__GNUC__) && defined(__clang__))
__attribute__ ((optnone))
#endif
char* FtpNlst(const char *outputfile, const char *path,	struct NetBuf *nControl) 
{
/* This original code is commented out due to the conversion changes */
//  return FtpXfer(outputfile, path, nControl, FTPLIB_DIR, FTPLIB_ASCII);
    FtpXfer(outputfile, path, nControl, FTPLIB_DIR, FTPLIB_ASCII);
    return rvs;
}

/*
 * FtpDir - issue a LIST command and write response to output
 *
 * return 1 if successful, 0 otherwise
 *err, no it doesn't, it returns a char* !
 */

#if (defined(__GNUC__) && !defined(__clang__))
__attribute__((optimize("O0")))
#endif

#if (!defined(__GNUC__) && defined(__clang__))
__attribute__ ((optnone))
#endif
char* FtpDir(const char *outputfile, const char *path, struct NetBuf *nControl) 
{
/* This original code is commented out due to the conversion changes */
//  return FtpXfer(outputfile, path, nControl, FTPLIB_DIR_VERBOSE, FTPLIB_ASCII);
    FtpXfer(outputfile, path, nControl, FTPLIB_DIR_VERBOSE, FTPLIB_ASCII);
    return rvs;
}

/*
 * FtpSize - determine the size of a remote file
 *
 * return 1 if successful, 0 otherwise
 */

#if (defined(__GNUC__) && !defined(__clang__))
__attribute__((optimize("O0")))
#endif

#if (!defined(__GNUC__) && defined(__clang__))
__attribute__ ((optnone))
#endif
int FtpSize(const char *path, int *size, char mode, struct NetBuf *nControl) 
{
    char cmd[256];
    int resp,sz,rv=1;

    if ((strlen(path) + 7) > sizeof(cmd))
        return 0;
    sprintf(cmd, "TYPE %c", mode);
    if (!FtpSendCmd(cmd, '2', nControl))
		return 0;
    sprintf(cmd,"SIZE %s",path);
    if (!FtpSendCmd(cmd,'2',nControl))
		rv = 0;
    else
    {
		if (sscanf(nControl->response, "%d %d", &resp, &sz) == 2)
			*size = sz;
		else
			rv = 0;
    }   
    return rv;
}

/*
 * FtpModDate - determine the modification date of a remote file
 *
 * return 1 if successful, 0 otherwise
 */
 
#if (defined(__GNUC__) && !defined(__clang__))
__attribute__((optimize("O0")))
#endif

#if (!defined(__GNUC__) && defined(__clang__))
__attribute__ ((optnone))
#endif
int FtpModDate(const char *path, char *dt, int max, struct NetBuf *nControl) 
{
    char buf[256];
    int rv = 1;

    if ((strlen(path) + 7) > sizeof(buf))
        return 0;
    sprintf(buf,"MDTM %s",path);
    if (!FtpSendCmd(buf,'2',nControl))
		rv = 0;
    else
		strncpy(dt, &nControl->response[4], max);

    return rv;
}

/*
 * FtpGet - issue a GET command and write received data to output
 *
 * return 1 if successful, 0 otherwise
 */

#if (defined(__GNUC__) && !defined(__clang__))
__attribute__((optimize("O0")))
#endif

#if (!defined(__GNUC__) && defined(__clang__))
__attribute__ ((optnone))
#endif
char* FtpGet(const char *outputfile, const char *path, char mode, struct NetBuf *nControl) 
{
/* This original code is commented out due to the conversion changes */
//  return FtpXfer(outputfile, path, nControl, FTPLIB_FILE_READ, mode);
    FtpXfer(outputfile, path, nControl, FTPLIB_FILE_READ, mode);
    return rvs;
}

/*
 * FtpPut - issue a PUT command and send data from input
 *
 * return 1 if successful, 0 otherwise
 */

#if (defined(__GNUC__) && !defined(__clang__))
__attribute__((optimize("O0")))
#endif

#if (!defined(__GNUC__) && defined(__clang__))
__attribute__ ((optnone))
#endif
int FtpPut(const char *inputfile, const char *path, char mode, struct NetBuf *nControl) 
{
/* This original code is commented out due to the conversion changes */
//  return FtpXfer(inputfile, path, nControl, FTPLIB_FILE_WRITE, mode);
    return FtpXfer(inputfile, path, nControl, FTPLIB_FILE_WRITE, mode);
}

/*
 * FtpRename - rename a file at remote
 *
 * return 1 if successful, 0 otherwise
 */

#if (defined(__GNUC__) && !defined(__clang__))
__attribute__((optimize("O0")))
#endif

#if (!defined(__GNUC__) && defined(__clang__))
__attribute__ ((optnone))
#endif
int FtpRename(const char *src, const char *dst, struct NetBuf *nControl) {
    char cmd[256];

    if (((strlen(src) + 7) > sizeof(cmd)) ||
        ((strlen(dst) + 7) > sizeof(cmd)))
        return 0;

    sprintf(cmd,"RNFR %s",src);
    if (!FtpSendCmd(cmd,'3',nControl))
		return 0;

    sprintf(cmd,"RNTO %s",dst);
    if (!FtpSendCmd(cmd,'2',nControl))
		return 0;

    return 1;
}

/*
 * FtpDelete - delete a file at remote
 *
 * return 1 if successful, 0 otherwise
 */

#if (defined(__GNUC__) && !defined(__clang__))
__attribute__((optimize("O0")))
#endif

#if (!defined(__GNUC__) && defined(__clang__))
__attribute__ ((optnone))
#endif
char* FtpDelete(const char *fnm, struct NetBuf *nControl) 
{
    char cmd[256];

    if ((strlen(fnm) + 7) > sizeof(cmd))
        return 0;
    sprintf(cmd,"DELE %s",fnm);
    if (!FtpSendCmd(cmd,'2', nControl))
		return "0";

    return "1";
}

/*
 * FtpQuit - disconnect from remote
 *
 * return 1 if successful, 0 otherwise
 */

#if (defined(__GNUC__) && !defined(__clang__))
__attribute__((optimize("O0")))
#endif

#if (!defined(__GNUC__) && defined(__clang__))
__attribute__ ((optnone))
#endif
void FtpQuit(struct NetBuf *nControl) 
{
    if (nControl->dir != FTPLIB_CONTROL)
		return;

    FtpSendCmd("QUIT",'2',nControl);
    net_close(nControl->handle);
    TGDSARM9Free(nControl->buf);
    TGDSARM9Free(nControl);
}
