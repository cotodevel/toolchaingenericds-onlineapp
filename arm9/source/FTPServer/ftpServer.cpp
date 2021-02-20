#include "ftpServer.h"

#include "typedefsTGDS.h"
#include "dsregs.h"
#include "dsregs_asm.h"

#include <in.h>
#include "ftpMisc.h"
#include "dswnifi_lib.h"

#include "fileBrowse.h"	//generic template functions from TGDS: maintain 1 source, whose changes are globally accepted by all TGDS Projects.

#include "FTPClientLib.h"
#include "FTPClientMisc.h"

bool FTPActiveMode = false;

//server ctx for sock1
//client ctx for sock2
//server_datasck_sain ctx for FTP PASSIVE Mode DataPort ran from Server (DS). (Client only connects to it)
//client_datasck_sain ctx for FTP ACTIVE Mode Dataport ran from Client. (Server DS only connects to it)
struct sockaddr_in server, client, server_datasck_sain, client_datasck_sain;
struct stat obj;

//sock1 = Initial FTP port opened by Server (DS). Basic FTP commands are served through this port.
//sock2 = incoming connection context from the Client. Basically where to send cmds received from sock1.
int sock1 = -1, sock2 = -1;

//server_datasocket == the DATA port open by the Server (DS) whose commands are processed and sent to Client. Server generates and listens cmds through that port.
int server_datasocket = -1;

//client_datasocket == the DATA port open by the Client whose commands are processed and sent to Server (DS). Client generates and listens cmds through that port.
int client_datasocket = -1;
int client_datasocketPortNumber = -1;
char client_datasocketIP[MAX_TGDSFILENAME_LENGTH+1];


char buf[100], command[5], filename[20];
int k, size, srv_len, cli_len = 0, c;
int filehandle;
bool globaldatasocketEnabled = false;

void ftpInit(bool isFTPServer){
	//FTP Server mode
	if(isFTPServer == true){
		strcpy((char*)CWDFTP,TGDSDirectorySeparator);
		setFTPState(FTP_SERVER_IDLE);
	}
	
	//FTP Client mode
	else{
		strcpy((char*)CWDFTP,TGDSDirectorySeparator);
		setFTPState(FTP_CLIENT_IDLE);
	}
}

u32 FTPServerService(){
	u32 curFTPStatus = 0;
	switch(getFTPState()){
		
		//FTP Server handshake 
		case(FTP_SERVER_IDLE):{
			switch_dswnifi_mode(dswifi_idlemode);
			connectDSWIFIAP(DSWNIFI_ENTER_WIFIMODE);
			if(sock1 != -1){
				disconnectAsync(sock1);
			}
			if(server_datasocket != -1){
				disconnectAsync(server_datasocket);
			}
			globaldatasocketEnabled = false;
			sock1 = openServerSyncConn(FTP_SERVER_SERVICE_PORT, &server);	//DS Server: listens at port FTP_SERVER_SERVICE_PORT now. Further access() through this port will come from a client.
			char IP[16];
			printf("[FTP Server:%s:%d]", print_ip((uint32)Wifi_GetIP(), IP), FTP_SERVER_SERVICE_PORT);
			printf("Waiting for connection:");
			setFTPState(FTP_SERVER_CONNECTING);
			curFTPStatus = FTP_SERVER_ACTIVE;
		}
		break;
	
		case(FTP_SERVER_CONNECTING):{
			printf("[Waiting for client...]");
			memset(&client, 0, sizeof(struct sockaddr_in));
			cli_len = sizeof(client);
			sock2 = accept(sock1,(struct sockaddr*)&client, &cli_len);
			
			//if blocking == will hang when recv so can't use that
			//int i=1;
			//ioctl(sock2, FIONBIO,&i); // Client socket is non blocking
			
			//int j=1;
			//ioctl(sock1, FIONBIO,&j); // Server socket is non blocking
			
			//Wait for client: FTP Server -> FTP Client Init session.
			printf("[Client Connected:%s:%d]",inet_ntoa(client.sin_addr), ntohs(client.sin_port));
			
			ftpResponseSender(sock2, 200, "hello");
			setFTPState(FTP_SERVER_ACTIVE);
			curFTPStatus = FTP_SERVER_ACTIVE;
		}
		break;
		case(FTP_SERVER_ACTIVE):{
			//Actual FTP Service			
			char buffer[MAX_TGDSFILENAME_LENGTH] = {0};
			int res = recv(sock2, buffer, sizeof(buffer), 0);
			int sendResponse = 0;
			if(res > 0){
				int len = strlen(buffer);
				if(len > 3){
					if(!strncmp(buffer, "USER", 4)){
						sendResponse = ftp_cmd_USER(sock2, 200, "password ?");
					}
					else if(!strncmp(buffer, "PASS", 4)){
						sendResponse = ftp_cmd_PASS(sock2, 200, "ok");
					}
					else if(!strncmp(buffer, "AUTH", 4)){
						sendResponse = ftpResponseSender(sock2, 504, "AUTH not supported");
					}
					else if(!strncmp(buffer, "BYE", 3) || !strncmp(buffer, "QUIT", 4)){
						printf("FTP server quitting.. ");
						int i = 1;
						sendResponse = send(sock2, &i, sizeof(int), 0);
					}
					else if(!strncmp(buffer, "STOR", 4)){
						sendResponse = ftp_cmd_STOR(sock2, 0, buffer);
					}
					else if(!strncmp(buffer, "RETR", 4)){
						sendResponse = ftp_cmd_RETR(sock2, 0, buffer);
					}
					//default unsupported, accordingly by: https://tools.ietf.org/html/rfc2389
					else if(!strncmp(buffer, "FEAT", 4)){
						sendResponse = ftp_cmd_FEAT(sock2, 211, "no-features");
					}
					//default unsupported, accordingly by: https://cr.yp.to/ftp/syst.html
					else if(!strncmp(buffer, "SYST", 4)){
						sendResponse = ftp_cmd_SYST(sock2, 215, "UNIX Type: L8");
					}
					//TYPE: I binary data by default
					else if(!strncmp(buffer, "TYPE", 4)){
						sendResponse = ftp_cmd_TYPE(sock2, 200, "Switching to Binary mode.");
					}
					else if(!strncmp(buffer, "CDUP", 4)){
						ftp_cmd_CDUP(sock2, 0, "");
					}
					else if(!strncmp(buffer, "PORT", 4)){
						//Connect to Client IP/Port here (DS is Client @ Client Data Port)
						char clientIP[MAX_TGDSFILENAME_LENGTH] = {0};
						char *theIpAndPort;
						int ipAddressBytes[4];
						int portBytes[2];
						theIpAndPort = getFtpCommandArg("PORT", buffer, 0);    
						sscanf(theIpAndPort, "%d,%d,%d,%d,%d,%d", &ipAddressBytes[0], &ipAddressBytes[1], &ipAddressBytes[2], &ipAddressBytes[3], &portBytes[0], &portBytes[1]);
						int ClientConnectionDataPort = ((portBytes[0]*256)+portBytes[1]);
						sprintf(clientIP, "%d.%d.%d.%d", ipAddressBytes[0],ipAddressBytes[1],ipAddressBytes[2],ipAddressBytes[3]);
						
						FTPActiveMode = true;	//enter FTP active mode // //data->clients[socketId].workerData.passiveModeOn = 0; //data->clients[socketId].workerData.activeModeOn = 1;
						
						//prepare client socket (IP/port) for later access
						client_datasocketPortNumber = ClientConnectionDataPort;
						strcpy(client_datasocketIP , clientIP);
						
						sendResponse = ftp_cmd_USER(sock2, 200, "PORT command successful.");
					}
					//PASV: mode that opens a data port aside the current server port, so binary data can be transfered through it.
					else if(!strncmp(buffer, "PASV", 4)){
						FTPActiveMode = false;	//enter FTP passive mode
						//printf("PASV > set data transfer port @ %d", FTP_SERVER_SERVICE_DATAPORT);
						char buf[MAX_TGDSFILENAME_LENGTH] = {0};
						int currentIP = (int)Wifi_GetIP();
						int dataPort = (int)FTP_SERVER_SERVICE_DATAPORT;
						
						int buggedOctet0 = (int)(currentIP&0xFF);
						if(buggedOctet0 < 0){
							buggedOctet0 +=256;
						}
						int buggedOctet1 = (int)((currentIP>>8)&0xFF);
						if(buggedOctet1 < 0){
							buggedOctet1 +=256;
						}
						int buggedOctet2 = (int)(((currentIP>>16)&0xFF));
						if(buggedOctet2 < 0){
							buggedOctet2 +=256;
						}
						int buggedOctet3 = (int)(currentIP>>24);
						if(buggedOctet3 < 0){
							buggedOctet3 +=256;
						}
						sprintf(buf, "Entering Passive Mode (%d,%d,%d,%d,%d,%d).", (int)buggedOctet0, (int)buggedOctet1, (int)buggedOctet2, (int)buggedOctet3, (int)(dataPort>>8), (int)(dataPort&0xFF));
						sendResponse = ftp_cmd_PASV(sock2, 227, buf);
					}
					else if(!strncmp(buffer, "LIST", 4)){
						sendResponse = ftp_cmd_LIST(sock2, 0, buffer);
					}
					else if(!strncmp(buffer, "DELE", 4)){
						char * fname = getFtpCommandArg("DELE", buffer, 0);
						char tmpBuf[MAX_TGDSFILENAME_LENGTH+1];
						strcpy(tmpBuf, fname);
						parsefileNameTGDS(tmpBuf);
						string fnameRemote = (string(tmpBuf));
						int retVal = remove(fnameRemote.c_str());
						if(retVal==0){
							sendResponse = ftpResponseSender(sock2, 250, "Delete Command successful.");
						}
						else{
							sendResponse = ftpResponseSender(sock2, 450, "Delete Command error.");
						}
						printf("trying DELE: [%s] ret:[%d]", fnameRemote.c_str(),retVal);
					}
					else if(!strncmp(buffer,"GET", 3)){
						sendResponse = ftpResponseSender(sock2, 502, "invalid command");//todo
						/*
						sscanf(buf, "%s%s", filename, filename);
						stat(filename, &obj);
						filehandle = open(filename, O_RDONLY);
						size = obj.st_size;
						if(filehandle == -1)
							size = 0;
						sendResponse = send(sock2, &size, sizeof(int), 0);
						if(size){
							sendfile(sock2, filehandle, NULL, size);
						}
						*/
					}
					else if(!strncmp(buffer, "PUT", 3)){
						printf("put command! ");
						sendResponse = ftpResponseSender(sock2, 502, "invalid command");//todo
						/*
						int c = 0;
						char *f;
						sscanf(buf+strlen(command), "%s", filename);
						recv(sock2, &size, sizeof(int), 0);
						int i = 1;
						while(1)
						{
							filehandle = open(filename, O_CREAT | O_EXCL | O_WRONLY, 0666);
							if(filehandle == -1)
							{
								sprintf(filename + strlen(filename), "%d", i);
							}
							else
								break;
						}
						f = TGDSARM9Malloc(size);
						recv(sock2, f, size, 0);
						c = write(filehandle, f, size);
						close(filehandle);
						sendResponse = send(sock2, &c, sizeof(int), 0);
						*/
					}
					else if(!strncmp(buffer, "CWD", 3)){
						char * CurrentWorkingDirectory = (char*)CWDFTP;
						char *pathToEnter;
						pathToEnter = getFtpCommandArg("CWD", buffer, 0);
						if(chdir((char*)pathToEnter) != 0) {
							printf(" CWD fail => %s ", pathToEnter);
							return ftpResponseSender(sock2, 550, "Error changing directory");
						}
						else{
							printf("1 CWD OK => %s ", pathToEnter);
							strcpy(CurrentWorkingDirectory, pathToEnter);
							return ftpResponseSender(sock2, 250, "Directory successfully changed.");
						}
					}
					//print working directory
					else if(!strncmp(buffer, "PWD", 3)){
						char * CurrentWorkingDirectory = (char*)CWDFTP;
						sendResponse = ftp_cmd_PWD(sock2, 257, CurrentWorkingDirectory);
					}
					else if(!strncmp(buffer, "MKD", 3)){
						sendResponse = ftpResponseSender(sock2, 502, "invalid command");//todo
					}
					else if(!strncmp(buffer, "RMD", 3)){
						sendResponse = ftpResponseSender(sock2, 502, "invalid command");//todo
					}
					else if(!strncmp(buffer, "LS", 2)){
						printf("ls command!");
						/*
						system("ls >temps.txt");
						stat("temps.txt",&obj);
						size = obj.st_size;
						sendResponse = send(sock2, &size, sizeof(int),0);
						filehandle = open("temps.txt", O_RDONLY);
						//sendfile(sock2,filehandle,NULL,size);
						*/
						sendResponse = ftpResponseSender(sock2, 502, "invalid command");//todo
					}
					else if(!strncmp(buffer, "CD", 2)){
						if(chdir((buffer+3)) == 0){
							c = 1;
						}
						else{
							c = 0;
						}
						sendResponse = send(sock2, &c, sizeof(int), 0);
					}
					else{
						printf("unhandled command: [%s]",buffer);
						sendResponse = ftpResponseSender(sock2, 502, "invalid command");
					}
				}
			}
			else{
				closeFTPDataPort(sock2);	//sock2, client disconnected, thus server closes its port.
				return FTP_SERVER_CLIENT_DISCONNECTED;
			}
			curFTPStatus = FTP_SERVER_ACTIVE;
		}
		break;
		
		
		//FTP Client handshake
		case(FTP_CLIENT_IDLE):{
			
			setFTPState(FTP_CLIENT_CONNECTING);
			curFTPStatus = FTP_CLIENT_ACTIVE;
		}
		break;
		
		case(FTP_CLIENT_CONNECTING):{
			printf("FTP Client Start. Init WIFI.");
			
			//FTP start
			switch_dswnifi_mode(dswifi_idlemode);
			connectDSWIFIAP(DSWNIFI_ENTER_WIFIMODE);
			if(sock1 != -1){
				disconnectAsync(sock1);
			}
			if(server_datasocket != -1){
				disconnectAsync(server_datasocket);
			}
			
			char * FTPHostAddress = "ftp.byethost7.com";
			char * FTPUser = "b7_27976645";
			char * FTPPass = "Lica8989";
			
			bool connected = false;
			
			if( !FtpConnect(FTPHostAddress,&conn))
			{
				connected = false;
				//errorMsg += "ftpcon, ";
			}
			else connected = true;
			printf("CONNECT OK");
			
			if(connected && !FtpLogin(FTPUser,FTPPass, conn))
			{
				printf("LOGIN FAIL. WRONG USER/PASS");
				connected = false;
				//errorMsg += "ftpLogin.";
			}
			else{
				printf("LOGIN OK CTM");
			}
			
			if( connected )
			{
				printf("FTP Server connect OK!!");
				//Getting the local file listing
				localDir = get_dir("/");
				//getFileListing(localDir, localFiles);

				//Getting the remote file listing
				remoteDir = get_remote_dir(NULL, conn);
				//getFileListing(remoteDir, remoteFiles);
				if( FtpPwd(curRemote, 256, conn) )
				{
					
				}
				
			}
			else
			{
				printf("Error connecting to FTP Server....");
			}
			
			setFTPState(FTP_CLIENT_ACTIVE);
			curFTPStatus = FTP_CLIENT_ACTIVE;
		}
		break;
		
		case(FTP_CLIENT_ACTIVE):{
			/*
			//Actual FTP Service			
			char buffer[MAX_TGDSFILENAME_LENGTH] = {0};
			int res = recv(sock1, buffer, sizeof(buffer), 0);
			int sendResponse = 0;
			if(res > 0){
				int len = strlen(buffer);
				if(len > 3){
					printf("unhandled command: [%s]",buffer);
					sendResponse = ftpResponseSender(sock1, 502, "invalid command");
				}
			}
			*/
			curFTPStatus = FTP_CLIENT_ACTIVE;
		}
		break;
		
		
		
	}
	return curFTPStatus;
}

