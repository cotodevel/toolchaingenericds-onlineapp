//FTP server
#include <string.h>
#include <stdio.h>
#include <in.h>
#include "ftpServer.h"
#include "ftpMisc.h"
#include "main.h"
#include "sgIP_Config.h"
#include "biosTGDS.h"
#include "dswnifi_lib.h"
#include "fileBrowse.h"	//generic template functions from TGDS: maintain 1 source, whose changes are globally accepted by all TGDS Projects.

//current working directory
volatile char CWDFTP[MAX_TGDSFILENAME_LENGTH+1];

//FTP Command implementation.
int ftp_cmd_USER(int s, int cmd, char* arg)
{
	return ftpResponseSender(s, cmd, arg);
}

int ftp_cmd_PASS(int s, int cmd, char* arg)
{
	return ftpResponseSender(s, cmd, arg);
}

int ftp_cmd_PWD(int s, int cmd, char* arg){
	return ftpResponseSender(s, cmd, arg);
}

int ftp_cmd_SYST(int s, int cmd, char* arg){
	return ftpResponseSender(s, cmd, arg);
}

int ftp_cmd_FEAT(int s, int cmd, char* arg){
	return ftpResponseSender(s, cmd, arg);
}

int ftp_cmd_TYPE(int s, int cmd, char* arg){
	return ftpResponseSender(s, cmd, arg);
}

int ftp_cmd_PASV(int s, int cmd, char* arg){
	return ftpResponseSender(s, cmd, arg);
}

int ftp_cmd_RETR(int s, int cmd, char* arg){
	char * fname = getFtpCommandArg("RETR", arg, 0); 
	char tmpBuf[MAX_TGDSFILENAME_LENGTH+1];
	memset(tmpBuf, 0, sizeof(tmpBuf));
	strcpy(tmpBuf, fname);
	parsefileNameTGDS(tmpBuf);
	
	//Prevent "/0:" bad filename parsing when using POSIX FS
	if(
		(tmpBuf[0] == '/') &&
		(tmpBuf[1] == '0') &&
		(tmpBuf[2] == ':')
	){
		char tempnewDiroutPath2[MAX_TGDSFILENAME_LENGTH+1];
		memcpy(tempnewDiroutPath2, &tmpBuf[1], strlen(tmpBuf)+1);
		tempnewDiroutPath2[strlen(tmpBuf)+2] = '\0';
		memset(tmpBuf, 0, sizeof(tmpBuf));
		strcpy(tmpBuf, tempnewDiroutPath2);
	}
	
	//if missing 0:/ add it
	if(
		(tmpBuf[0] != '0') &&
		(tmpBuf[1] != ':') &&
		(tmpBuf[2] != '/')
	){
		char tempnewDiroutPath2[MAX_TGDSFILENAME_LENGTH+1];
		memset(tempnewDiroutPath2, 0, sizeof(tempnewDiroutPath2));
		strcpy(tempnewDiroutPath2, getfatfsPath((sint8 *)tmpBuf));
		memset(tmpBuf, 0, sizeof(tmpBuf));
		strcpy(tmpBuf, tempnewDiroutPath2);
	}
	printf("RETR cmd: %s",tmpBuf);
	//Open Data Port for FTP Server so Client can connect to it (FTP Passive Mode)
	struct sockaddr_in clientAddr;
	int clisock = openAndListenFTPDataPort(&clientAddr);
	int sendResponse = ftpResponseSender(s, 150, "Opening BINARY mode data connection for retrieve file from server.");
	
	if(clisock >= 0){
		int total_len = FS_getFileSize((char*)tmpBuf);
		FILE * fh = fopen(tmpBuf, "r");
		
		if(fh != NULL){
			//retrieve from server, to client.
			int written = send_file(clisock, fh, total_len);
			//printf("To client: file written %d bytes", written);
			disconnectAsync(clisock);
			fclose(fh);
			sendResponse = ftpResponseSender(s, 226, "Transfer complete.");
		}
		//could not open file
		else{
			printf("RETR file %s open ERROR",tmpBuf);
			sendResponse = ftpResponseSender(s, 451, "Could not open file.");
		}
	}
	else{
		sendResponse = ftpResponseSender(s, 425, "Connection closed; transfer aborted.");
	}
	return sendResponse;
}



int ftp_cmd_STOR(int s, int cmd, char* arg){
	char * fname = getFtpCommandArg("STOR", arg, 0); 
	char tmpBuf[MAX_TGDSFILENAME_LENGTH+1];
	sprintf(tmpBuf, "%s%s", "0:/", fname);
	parsefileNameTGDS(tmpBuf);
	printf("STOR cmd: %s",tmpBuf);
	
	swiDelay(8888);
	
	//Open Data Port for FTP Server so Client can connect to it (FTP Passive Mode)
	struct sockaddr_in clientAddr;
	int clisock = openAndListenFTPDataPort(&clientAddr);
	int sendResponse = ftpResponseSender(s, 150, "Opening BINARY mode data connection for retrieve file from server.");
	
	swiDelay(8888);
	
	if(clisock >= 0){
		int total_len = FS_getFileSize((char*)tmpBuf);
		FILE * fh = fopen(tmpBuf, "w+");
		
		if(fh != NULL){
			//retrieve data from client socket.
			char * client_reply = (char*)TGDSARM9Malloc(SENDRECVBUF_SIZE);
			int received_len = 0;
			int total_len = 0;
			while( ( received_len = recv(clisock, client_reply, sizeof(client_reply), 0 ) ) != 0 ) { // if recv returns 0, the socket has been closed.
				if(received_len>0) { // data was received!
					total_len += received_len;
					fwrite(client_reply , 1, received_len , fh);
					//printf("Received byte size = %d", received_len);
				}
				swiDelay(1);
			}
			sint32 FDToSync = fileno(fh);
			fsync(FDToSync);	//save TGDS FS changes right away
			fclose(fh);
			
			char tmpName[256];
			char ext[256];
			
			strcpy(tmpName, tmpBuf);
			separateExtension(tmpName, ext);
			strlwr(ext);
			
			//Boot .NDS file! (homebrew only)
			if(strncmp(ext,".nds", 4) == 0){
				fillNDSLoaderContext((char*)tmpBuf);
			}
			
			TGDSARM9Free(client_reply);
			disconnectAsync(clisock);
			sendResponse = ftpResponseSender(s, 226, "Transfer complete.");
			swiDelay(8888);
		}
		//could not open file
		else{
			printf("STOR file %s open ERROR",tmpBuf);
			sendResponse = ftpResponseSender(s, 451, "Could not open file.");
			swiDelay(8888);
		}
	}
	else{
		sendResponse = ftpResponseSender(s, 425, "Connection closed; transfer aborted.");
		swiDelay(8888);
	}
	return sendResponse;
}

int ftp_cmd_CDUP(int s, int cmd, char* arg){	
	int sendResponse = 0;
	bool cdupStatus = leaveDir((char*)CWDFTP);
	if(cdupStatus == true){
		sendResponse = ftpResponseSender(s, 200, "OK");
	}
	else{
		sendResponse = ftpResponseSender(s, 550, "ERROR");
	}
	return sendResponse;
}


uint32 CurFTPState = 0;
u32 getFTPState(){
	return CurFTPState;
}

void setFTPState(uint32 FTPState){
	CurFTPState = FTPState;
} 

int ftpResponseSender(int s, int n, char* mes){
	volatile char data[MAX_TGDSFILENAME_LENGTH];
	sprintf((char*)data, "%d %s \n", n, mes);
	return send(s, (char*)&data[0], strlen((char*)&data[0]), 0);
}

int currserverDataListenerSock = -1;

//These two open/close a FTP Server (Passive Mode) Data Port
int openAndListenFTPDataPort(struct sockaddr_in * sain){

	int cliLen = sizeof(struct sockaddr_in);
	int serverDataListenerSock = openServerSyncConn(FTP_SERVER_SERVICE_DATAPORT, sain);
	currserverDataListenerSock = serverDataListenerSock;
	
	if(serverDataListenerSock == -1){
		printf("failed allocing serverDataListener");
		while(1==1){}
	}
	
	int clisock = accept(serverDataListenerSock, (struct sockaddr *)sain, &cliLen);
	if(clisock == -1){
		printf("failed allocing incoming client");
		while(1==1){}
	}
	
	if(clisock > 0) {
		printf("FTP Server (DataPort): Got a connection from:");
		printf("[%s:%d]", inet_ntoa(sain->sin_addr), ntohs(sain->sin_port));
	}
	return clisock;
}

void closeFTPDataPort(int sock){
	disconnectAsync(sock);
	if(currserverDataListenerSock != 0){
		disconnectAsync(currserverDataListenerSock);
		currserverDataListenerSock = -1;
	}
}



bool send_all(int socket, void *buffer, size_t length, int * written)
{
    char *ptr = (char*) buffer;
    while (length > 0)
    {
        int i = send(socket, ptr, length, 0);
		swiDelay(100);
		if (i < 1) return false;
		ptr += i;
		(*written)+=i;
        length -= i;
    }
    return true;
}

int send_file(int peer, FILE *f, int fileSize) {
	char * filebuf = (char*)TGDSARM9Malloc(SENDRECVBUF_SIZE + 1024);
    int written = 0;
	int readSofar= 0;
	int lastwritten = 0;
    while((readSofar=fread(filebuf, 1, SENDRECVBUF_SIZE, f)) > 0) {
		int write = 0;
		if(send_all(peer, filebuf, readSofar, &write) == true){
			//printf("written %d bytes", write);
			written+=write;
			lastwritten = write;
		}
		else{
			//printf("failed to write %d bytes", readSofar);
		}

		//the last part must be resent
		if( (readSofar % SENDRECVBUF_SIZE) != 0 ){
			write = 0;
			int wtf = 5952 + 1024;	//wtf explanation: there is about 6K trimmed near the end of the end of the transfer regardless the complete binary was sent-checked byte by byte. Need to resend under a 1K buffer.
			memset(filebuf + lastwritten, 0 , (SENDRECVBUF_SIZE + 1024 - lastwritten));
			if(lastwritten > wtf){
				send_all(peer, filebuf + lastwritten - wtf + 1024, wtf, &write);
			}
		}
    }
	TGDSARM9Free(filebuf);
	
	u8 endByte=0x0;
	send(peer, &endByte, 1, 0);	//finish connection so Client disconnects.
	
    return written;
}


char *getFtpCommandArg(char * theCommand, char *theCommandString, int skipArgs)
{
    char *toReturn = theCommandString + strlen(theCommand);

   /* Pass spaces */ 
    while (toReturn[0] == ' ')
    {
        toReturn += 1;
    }

    /* Skip eventual secondary arguments */
    if(skipArgs == 1)
    {
        if (toReturn[0] == '-')
        {
            while (toReturn[0] != ' ' &&
                   toReturn[0] != '\r' &&
                   toReturn[0] != '\n' &&
                   toReturn[0] != 0)
                {
                    toReturn += 1;
                }
        }

        /* Pass spaces */ 
        while (toReturn[0] == ' ')
        {
            toReturn += 1;
        }
    }
	
	//is it a directory / file? If so, prevent "/0:" bad file/dir parsing from the FTP Client
	if(
		(toReturn[0] == '/') &&
		(toReturn[1] == '0') &&
		(toReturn[2] == ':')
	){
		toReturn += 1;
	}
	
    return toReturn;
}




int ftp_cmd_LIST(int s, int cmd, char* arg){

	//Open Data Port for FTP Server so Client can connect to it (FTP Passive Mode)
	struct sockaddr_in clientAddr;
	int clisock = openAndListenFTPDataPort(&clientAddr);
	int sendResponse = 0;
	if(clisock >= 0){
		swiDelay(8888);
		sendResponse = ftpResponseSender(s, 150, "Opening BINARY mode data connection for file list.");
		swiDelay(8888);
		
		//todo: filter different LIST args here
		char * LISTargs = getFtpCommandArg("LIST", arg, 0);  
		
		int fileList = 0;
		int dirList = 0;
		int curFileDirIndx = 0;
		
		char curPath[MAX_TGDSFILENAME_LENGTH+1];
		strcpy(curPath, (const char*)CWDFTP);
		
		//Create TGDS Dir API context
		struct FileClassList * fileClassListCtx = initFileList();
		cleanFileList(fileClassListCtx);
		
		int startFromIndex = 0;
		struct FileClass * fileClassInst = NULL;
		fileClassInst = FAT_FindFirstFile(curPath, fileClassListCtx, startFromIndex);
		
		while(fileClassInst != NULL){
			//directory?
			if(fileClassInst->type == FT_DIR){
				char tmpBuf[512];
				strcpy(tmpBuf, fileClassInst->fd_namefullPath);
				parseDirNameTGDS(tmpBuf);
				strcpy(fileClassInst->fd_namefullPath, tmpBuf);
				char buffOut[256+1] = {0};
				int fSizeDir = strlen(fileClassInst->fd_namefullPath);
				sprintf(buffOut, "drw-r--r-- 1 DS group          %d Feb  1  2009 %s \r\n", fSizeDir, fileClassInst->fd_namefullPath);
				send(clisock, (char*)buffOut, strlen(buffOut), 0);
				dirList++;
			}
			//file?
			else if(fileClassInst->type == FT_FILE){
				char tmpBuf[512];
				strcpy(tmpBuf, fileClassInst->fd_namefullPath);
				parsefileNameTGDS(tmpBuf);
				strcpy(fileClassInst->fd_namefullPath, tmpBuf);
				char buffOut[256+1] = {0};
				int fSizeFile = FS_getFileSize((char*)fileClassInst->fd_namefullPath);
				sprintf(buffOut, "-rw-r--r-- 1 DS group          %d Feb  1  2009 %s \r\n", fSizeFile, fileClassInst->fd_namefullPath);
				send(clisock, (char*)buffOut, strlen(buffOut), 0);
				fileList++;
				swiDelay(8888);
			}
			
			//more file/dir objects?
			fileClassInst = FAT_FindNextFile(curPath, fileClassListCtx);
			curFileDirIndx++;
		}
		
		//Free TGDS Dir API context
		freeFileList(fileClassListCtx);
		
		printf("Files:%d - Dirs:%d", fileList, dirList);
		
		u8 endByte=0x0;
		send(clisock, &endByte, 1, 0);
		swiDelay(8888);
		
		closeFTPDataPort(clisock);
		
		sendResponse = ftpResponseSender(s, 226, "Transfer complete.");
		swiDelay(8888);
	}
	else{
		sendResponse = ftpResponseSender(s, 426, "Connection closed; transfer aborted.");
		swiDelay(8888);
	}
	
	return sendResponse;
}