#ifndef __ftp_misc_h__
#define __ftp_misc_h__

#include "typedefsTGDS.h"
#include "dsregs.h"
#include "dsregs_asm.h"

#include "fatfslayerTGDS.h"

#ifdef __cplusplus

//C++ part
using namespace std;
#include <string>
#include <list>
#include <vector>

class FileDirEntry
{
  public:
    int Index;
	std::string filePathFilename;
    int type;	//FT_DIR / FT_FILE / FT_NONE	//  setup on Constructor / updated by getFileFILINFOfromPath(); / must be init from the outside 
    // Constructor
    FileDirEntry(int indexInst, std::string filePathFilenameInst, int typeInst)
	{
		Index = indexInst;
		filePathFilename = filePathFilenameInst;
		type = typeInst;
	}
	
	//helpers if/when Constructor is not available
	int getIndex()
    {
		return Index;
    }
    std::string getfilePathFilename()
    {
		return filePathFilename;
    }
	int gettype()
    {
		return type;
    }
	
	void setIndex(int IndexInst){
		Index = IndexInst;
	}
	void setfilename(std::string filePathFilenameInst){
		filePathFilename = filePathFilenameInst;
	}
	void settype(int typeInst){
		type = typeInst;
	}
};
#endif

//Internal FTP Server status
#define FTP_SERVER_IDLE (uint32)(0xffff1010)				//idle -> FTP Server waiting for FTP Client to connect.
#define FTP_SERVER_CONNECTING (uint32)(0xffff1011)			//FTP Server <--> FTP Client accept and start initial handshake 
#define FTP_SERVER_ACTIVE (uint32)(0xffff1013)				//FTP Server <--> FTP Client Connected session. If something fails it will disconnect here and status will be the below this one.
#define FTP_SERVER_CLIENT_DISCONNECTED (sint32)(0)			//FTP Server has just disconnected from FTP Client.

#define FTP_CLIENT_IDLE (uint32)(0xffff1014)	//FTP Client is idle
#define FTP_CLIENT_CONNECTING (uint32)(0xffff1015)	//FTP Client <--> FTP Server accept and start initial handshake 
#define FTP_CLIENT_ACTIVE (uint32)(0xffff1016)	//FTP Client <--> FTP Server Connected session. If something fails it will disconnect here and status will be the below this one.
#define FTP_CLIENT_DISCONNECTED_FROM_SERVER (uint32)(0xffff1017)	//FTP Client has just disconnected from FTP Server.


#define SENDRECVBUF_SIZE (int)(512*1024)

#endif

#ifdef __cplusplus
extern "C" {
#endif

extern int ftp_cmd_USER(int s, int cmd, char* arg);
extern int ftp_cmd_PASS(int s, int cmd, char* arg);
extern int ftp_cmd_PWD(int s, int cmd, char* arg);
extern int ftp_cmd_SYST(int s, int cmd, char* arg);
extern int ftp_cmd_FEAT(int s, int cmd, char* arg);
extern int ftp_cmd_TYPE(int s, int cmd, char* arg);
extern int ftp_cmd_PASV(int s, int cmd, char* arg);
extern int ftp_cmd_STOR(int s, int cmd, char* arg);
extern int ftp_cmd_RETR(int s, int cmd, char* arg);
extern int ftp_cmd_CDUP(int s, int cmd, char* arg);

extern uint32 CurFTPState;
extern u32 getFTPState();
extern void setFTPState(uint32 FTPState);
extern int ftpResponseSender(int s, int n, char* mes);

//These two open/close a new FTP Server Data Port (Passive Mode)
extern int openAndListenFTPDataPort(struct sockaddr_in * sain);
extern void closeFTPDataPort(int sock);
extern int currserverDataListenerSock;
extern bool send_all(int socket, void *buffer, size_t length, int * written);
extern int send_file(int peer, FILE *f, int fileSize);

extern char *getFtpCommandArg(char * theCommand, char *theCommandString, int skipArgs);
extern int ftp_cmd_LIST(int s, int cmd, char* arg);

//current working directory
extern volatile char CWDFTP[MAX_TGDSFILENAME_LENGTH+1];

#ifdef __cplusplus
}
#endif