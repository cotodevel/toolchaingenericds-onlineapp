#ifndef __ftpServer_h__
#define __ftpServer_h__

#include "typedefsTGDS.h"
#include "dsregs.h"
#include "dsregs_asm.h"
#include "fatfslayerTGDS.h"

#define FTP_SERVER_SERVICE_PORT 21
#define FTP_SERVER_SERVICE_DATAPORT (sint32)(20)

#endif

#ifdef __cplusplus
extern "C" {
#endif

extern int FTPServerService();
extern bool globaldatasocketEnabled;

//server ctx for sock1
//client ctx for sock2
//server_datasck_sain ctx for FTP PASSIVE Mode DataPort ran from Server (DS). (Client only connects to it)
//client_datasck_sain ctx for FTP ACTIVE Mode Dataport ran from Client. (Server DS only connects to it)
extern struct sockaddr_in server, client, server_datasck_sain, client_datasck_sain;
extern struct stat obj;

//sock1 = Initial FTP port opened by Server (DS). Basic FTP commands are served through this port.
//sock2 = incoming connection context from the Client. Basically where to send cmds received from sock1.
extern int sock1, sock2;

//server_datasocket == the DATA port open by the Server (DS) whose commands are processed and sent to Client. Server generates and listens cmds through that port.
extern int server_datasocket;

//client_datasocket == the DATA port open by the Client whose commands are processed and sent to Server (DS). Client generates and listens cmds through that port.
extern int client_datasocket;
extern int client_datasocketPortNumber;
extern char client_datasocketIP[MAX_TGDSFILENAME_LENGTH+1];

extern char buf[100], command[5], filename[20];
extern int k, i, size, srv_len,cli_len, c;
extern int filehandle;
extern bool FTPActiveMode;

extern void ftpInit();

#ifdef __cplusplus
}
#endif