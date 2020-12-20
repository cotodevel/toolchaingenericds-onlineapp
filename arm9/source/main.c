/*
			Copyright (C) 2017  Coto
This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301
USA

*/

#include "main.h"
#include "dsregs.h"
#include "dsregs_asm.h"
#include "typedefsTGDS.h"
#include "gui_console_connector.h"
#include "dswnifi_lib.h"
#include "fileBrowse.h"	//generic template functions from TGDS: maintain 1 source, whose changes are globally accepted by all TGDS Projects.
#include "TGDSLogoLZSSCompressed.h"
#include "ipcfifoTGDSUser.h"
#include "fatfslayerTGDS.h"
#include "cartHeader.h"
#include "ff.h"
#include "dldi.h"
#include "loader.h"
#include "arm7bootldr.h"
#include "ftpMisc.h"
#include "ftpServer.h"
#include "xenofunzip.h"

bool GDBEnabled = false;
char curChosenBrowseFile[MAX_TGDSFILENAME_LENGTH+1];

void menuShow(){
	clrscr();
	printf("     ");
	printf("     ");
	printf("ToolchainGenericDS-OnlineApp: ");
	printf("(Select): This menu. ");
	printf("(Start): FileBrowser : (A) Run .zip/.nds file ");
	printf("Available physical memory: %d >%d", getMaxRam(), TGDSPrintfColor_Yellow);
	printf("Available User Memory: %d >%d", TGDSMallocFreeMemory9(), TGDSPrintfColor_Yellow);
	printarm7DebugBuffer();
}

static inline void initNDSLoader(){
	coherent_user_range_by_size((uint32)(0x02000000), (0x02400000 - 0x02300000));
	dmaFillHalfWord(0, 0, (uint32)(0x02000000), (0x02400000 - 0x02300000));
	
	coherent_user_range_by_size((uint32)NDS_LOADER_DLDISECTION_CACHED, (48*1024) + (96*1024) + (64*1024) + (16*1024));
	dmaFillHalfWord(0, 0, (uint32)NDS_LOADER_DLDISECTION_CACHED, (48*1024) + (96*1024) + (64*1024) + (16*1024));
	
	//copy loader code (arm7bootldr.bin) to ARM7's EWRAM portion while preventing Cache issues
	coherent_user_range_by_size((uint32)&arm7bootldr[0], (int)arm7bootldr_size);					
	memcpy ((void *)NDS_LOADER_IPC_BOOTSTUBARM7_CACHED, (u32*)&arm7bootldr[0], arm7bootldr_size); 	//memcpy ( void * destination, const void * source, size_t num );	//memset(void *str, int c, size_t n)

	setNDSLoaderInitStatus(NDSLOADER_INIT_OK);
}

static u8 * outBuf7 = NULL;
static u8 * outBuf9 = NULL;

//generates a table of sectors out of a given file. It has the ARM7 binary and ARM9 binary
bool fillNDSLoaderContext(char * filename){
	
	FILE * fh = fopen(filename, "r");
	if(fh != NULL){
		
		int headerSize = sizeof(struct sDSCARTHEADER);
		u8 * NDSHeader = (u8 *)malloc(headerSize*sizeof(u8));
		if (fread(NDSHeader, 1, headerSize, fh) != headerSize){
			printf("header read error");
			free(NDSHeader);
			fclose(fh);
			return false;
		}
		printf("header parsed correctly.");
		struct sDSCARTHEADER * NDSHdr = (struct sDSCARTHEADER *)NDSHeader;
		
		//fill data into shared buffers
		int sectorSize = 0;
		if(FF_MAX_SS == FF_MIN_SS){
			sectorSize = (int)FF_MIN_SS;
		}
		else{
			#if (FF_MAX_SS != FF_MIN_SS)
			sectorSize = dldiFs.ssize;	//when dldiFs sectorsize is variable, it's 512 bytes per sector
			#endif
		}
		
		//Common
		int clusterSizeBytes = getDiskClusterSizeBytes();
		int sectorsPerCluster = dldiFs.csize;
		NDS_LOADER_IPC_CTX_UNCACHED->sectorsPerCluster = sectorsPerCluster;
		NDS_LOADER_IPC_CTX_UNCACHED->sectorSize = sectorSize;
		
		//ARM7
		int arm7BootCodeSize = NDSHdr->arm7size;
		u32 arm7BootCodeOffsetInFile = NDSHdr->arm7romoffset;
		NDS_LOADER_IPC_CTX_UNCACHED->arm7EntryAddress = NDSHdr->arm7entryaddress;
		NDS_LOADER_IPC_CTX_UNCACHED->bootCode7FileSize = arm7BootCodeSize;
		int sectorOffsetStart7 = arm7BootCodeOffsetInFile % sectorSize;
		int sectorOffsetEnd7 = (arm7BootCodeOffsetInFile + arm7BootCodeSize - sectorOffsetStart7) % sectorSize;
		NDS_LOADER_IPC_CTX_UNCACHED->sectorOffsetStart7 = sectorOffsetStart7;
		NDS_LOADER_IPC_CTX_UNCACHED->sectorOffsetEnd7 = sectorOffsetEnd7;
		
		//ARM9
		int arm9BootCodeSize = NDSHdr->arm9size;
		u32 arm9BootCodeOffsetInFile = NDSHdr->arm9romoffset;
		NDS_LOADER_IPC_CTX_UNCACHED->arm9EntryAddress = NDSHdr->arm9entryaddress;
		NDS_LOADER_IPC_CTX_UNCACHED->bootCode9FileSize = arm9BootCodeSize;
		int sectorOffsetStart9 = arm9BootCodeOffsetInFile % sectorSize;
		int sectorOffsetEnd9 = (arm9BootCodeOffsetInFile + arm9BootCodeSize - sectorOffsetStart9) % sectorSize;
		NDS_LOADER_IPC_CTX_UNCACHED->sectorOffsetStart9 = sectorOffsetStart9;
		NDS_LOADER_IPC_CTX_UNCACHED->sectorOffsetEnd9 = sectorOffsetEnd9;
		
		clrscr();
		printf("-");
		printf("-");
		
		//Get TGDS file handle
		int fdindex = fileno(fh);
		struct fd * fdinst = getStructFD(fdindex);
		
		//reset FP to make it sector-relative + get file size
		fseek(fh,0,SEEK_END);
		int fileSize = ftell(fh);
		fseek(fh,0,SEEK_SET);
		NDS_LOADER_IPC_CTX_UNCACHED->fileSize = fileSize;
		
		printf("fillNDSLoaderContext():");
		printf("arm7BootCodeSize:%d", arm7BootCodeSize);
		printf("arm7BootCodeOffsetInFile:%x", arm7BootCodeOffsetInFile);
		printf("arm7BootCodeEntryAddress:%x", NDS_LOADER_IPC_CTX_UNCACHED->arm7EntryAddress);
		printf("arm9BootCodeSize:%d", arm9BootCodeSize);
		printf("arm9BootCodeOffsetInFile:%x", arm9BootCodeOffsetInFile);
		printf("arm9BootCodeEntryAddress:%x", NDS_LOADER_IPC_CTX_UNCACHED->arm9EntryAddress);
		
		//Generate cluster filemap
		uint32_t* cluster_table = (uint32_t*)&NDS_LOADER_IPC_CTX_UNCACHED->sectorTableBootCode[0];
		uint32_t  cur_cluster = getStructFDFirstCluster(fdinst);
		while (cur_cluster >= 2 && cur_cluster != 0xFFFFFFFF)
		{
			*cluster_table = clst2sect(&dldiFs, cur_cluster);
			cluster_table++;
			cur_cluster = f_getFat(fdinst->filPtr, cur_cluster);
		}
		*(cluster_table) = 0xFFFFFFFF;
		
		printf("NDSLoader start. ");
		
		u8 * outBuf = (u8 *)malloc(sectorSize * sectorsPerCluster);
		
		//Uncached to prevent cache issues right at once
		outBuf7 = (u8 *)(NDS_LOADER_IPC_ARM7BIN_UNCACHED);	//will not be higher than: arm7BootCodeSize
		outBuf9 = (u8 *)(NDS_LOADER_IPC_CTX_UNCACHED->arm9EntryAddress | 0x400000); //will not be higher than: arm9BootCodeSize or 0x2D0000 (2,949,120 bytes)
		
		u8 * outBuf7Seek = outBuf7;
		u8 * outBuf9Seek = outBuf9;
		
		int globalPtr = 0; //this one maps the entire file in 512 bytes (sectorSize)
		u32 cur_clustersector = NDS_LOADER_IPC_CTX_UNCACHED->sectorTableBootCode[0];
		uint32_t data_max = (uint32_t)(fileSize);
		uint32_t data_read = 0;
		while((cur_clustersector != 0xFFFFFFFF) && ((data_read * (sectorSize * sectorsPerCluster)) < data_max) )
		{
			//full sector copy
			memset(outBuf, 0, sectorSize * sectorsPerCluster);
			
			dldi_handler_read_sectors(cur_clustersector, sectorsPerCluster, (void*)(outBuf));
			
			//for each sector per cluster...
			int i = 0;
			for(i = 0; i < sectorsPerCluster; i++){
				
				//copy it into output ARM7 binary
				if ( (globalPtr >= arm7BootCodeOffsetInFile) && (globalPtr < (arm7BootCodeOffsetInFile+arm7BootCodeSize)) ){
					//last part?
					if( ((arm7BootCodeOffsetInFile+arm7BootCodeSize) - globalPtr) > sectorOffsetEnd7){
						memcpy (outBuf7Seek, outBuf + (i*512), 512);
						outBuf7Seek+=512;
					}
					else{
						memcpy (outBuf7Seek, outBuf + (i*512), sectorOffsetEnd7);
						outBuf7Seek+=sectorOffsetEnd7;
					}
				}
				
				//copy it into output ARM9 binary
				if ( (globalPtr >= arm9BootCodeOffsetInFile) && (globalPtr < (arm9BootCodeOffsetInFile+arm9BootCodeSize)) ){
					//last part?
					if( ((arm9BootCodeOffsetInFile+arm9BootCodeSize) - globalPtr) > sectorOffsetEnd9){
						memcpy (outBuf9Seek, outBuf + (i*512), 512);
						outBuf9Seek+=512;
					}
					else{
						memcpy (outBuf9Seek, outBuf + (i*512), sectorOffsetEnd9);
						outBuf9Seek+=sectorOffsetEnd9;
					}
				}	
				globalPtr +=512;
			}
			
			//ARM7 Range check
			data_read++;
			cur_clustersector = (u32)NDS_LOADER_IPC_CTX_UNCACHED->sectorTableBootCode[data_read];
		}
		
		printf("ARM7 %d bytes. [Addr: %x]", arm7BootCodeSize, (outBuf7 - 0x400000));
		printf("ARM9 %d bytes. [Addr: %x]", arm9BootCodeSize, (outBuf9 - 0x400000));
		
		//Build NDS Header
		memcpy((u8*)0x027FFE00, NDSHeader, (headerSize*sizeof(u8)) );
		
		printf("NDSLoader end. ");
		
		free(outBuf);
		free(NDSHeader);
		fclose(fh);
		int ret=FS_deinit();
		
		asm("mcr	p15, 0, r0, c7, c10, 4");
		WRAM_CR = WRAM_0KARM9_32KARM7;	//96K ARM7 : 0x037f8000 ~ 0x03810000
		flush_icache_all();
		flush_dcache_all();
		//Copy and relocate current TGDS DLDI section into target ARM9 binary
		bool stat = dldiPatchLoader((data_t *)NDS_LOADER_IPC_CTX_UNCACHED->arm9EntryAddress, (u32)arm9BootCodeSize, (u32)&_io_dldi_stub);
		if(stat == false){
			printf("DLDI Patch failed. APP does not support DLDI format.");
		}
		
		runBootstrapARM7();	//ARM9 Side						/	
		setNDSLoaderInitStatus(NDSLOADER_LOAD_OK);	//		|	Wait until ARM7.bin is copied back to IWRAM's target address
		waitWhileNotSetStatus(NDSLOADER_START);		//		\
		
		//reload ARM9.bin
		coherent_user_range_by_size((uint32)NDS_LOADER_IPC_CTX_UNCACHED->arm9EntryAddress, (u32)arm9BootCodeSize);
		reloadARMCore(NDS_LOADER_IPC_CTX_UNCACHED->arm9EntryAddress);
	}
	return false;
}

int main(int argc, char argv[argvItems][MAX_TGDSFILENAME_LENGTH]) {
	
	/*			TGDS 1.6 Standard ARM9 Init code start	*/
	bool isTGDSCustomConsole = false;	//set default console or custom console: default console
	GUI_init(isTGDSCustomConsole);
	GUI_clear();
	bool isCustomTGDSMalloc = false;
	setTGDSMemoryAllocator(getProjectSpecificMemoryAllocatorSetup(TGDS_ARM7_MALLOCSTART, TGDS_ARM7_MALLOCSIZE, isCustomTGDSMalloc));
	sint32 fwlanguage = (sint32)getLanguage();
	
	printf("     ");
	printf("     ");
	
	#ifdef ARM7_DLDI
	setDLDIARM7Address((u32 *)TGDSDLDI_ARM7_ADDRESS);	//Required by ARM7DLDI!
	#endif
	
	int ret=FS_init();
	if (ret == 0)
	{
		printf("FS Init ok.");
	}
	else if(ret == -1)
	{
		printf("FS Init error.");
	}
	switch_dswnifi_mode(dswifi_idlemode);
	asm("mcr	p15, 0, r0, c7, c10, 4");
	flush_icache_all();
	flush_dcache_all();
	/*			TGDS 1.6 Standard ARM9 Init code end	*/
	
	initNDSLoader();	//set up NDSLoader
	
	//load TGDS Logo (NDS BMP Image)
	//VRAM A Used by console
	//VRAM C Keyboard and/or TGDS Logo
	
	//Show logo
	RenderTGDSLogoMainEngine((uint8*)&TGDSLogoLZSSCompressed[0], TGDSLogoLZSSCompressed_size);
	
	//Remove logo and restore Main Engine registers
	//restoreFBModeMainEngine();
	
	menuShow();
	ftpInit();
	//fillNDSLoaderContext((char*)"0:/ToolchainGenericDS-multiboot.nds");
	
	while (1){
		sint32 FTP_SERVER_STATUS = FTPServerService();
		switch(FTP_SERVER_STATUS){
			//Server Running
			case(FTP_SERVER_ACTIVE):{
				
			}
			break;
			
			//Server Disconnected/Idle!
			case(FTP_SERVER_CLIENT_DISCONNECTED):{				
				/*
				closeFTPDataPort(sock1);
				setFTPState(FTP_SERVER_IDLE);
				printf("Client disconnected!. Press A to retry.");
				switch_dswnifi_mode(dswifi_idlemode);
				scanKeys();
				while(!(keysPressed() & KEY_A)){
					scanKeys();
					IRQVBlankWait();
				}
				main(argc, argv);
				*/
			}
			break;
		}
		
		scanKeys();
		if (keysPressed() & KEY_START){
			char startPath[MAX_TGDSFILENAME_LENGTH+1];
			strcpy(startPath,"/");
			while( ShowBrowser((char *)startPath, (char *)&curChosenBrowseFile[0]) == true ){	//as long you keep using directories ShowBrowser will be true
				
			}
			
			scanKeys();
			while((keysPressed() & KEY_A) || (keysPressed() & KEY_START)){
				scanKeys();
			}
			
			//Send args
			printf("[Booting %s]", curChosenBrowseFile);
			printf("Want to send argument?");
			printf("(A) Yes: (Start) Choose arg.");
			printf("(B) No. ");
			
			while(1==1){
				scanKeys();
				if(keysPressed()&KEY_A){
					scanKeys();
					while(keysPressed() & KEY_A){
						scanKeys();
					}
					
					char argv0[MAX_TGDSFILENAME_LENGTH+1];
					memset(argv0, 0, sizeof(argv0));
					
					while( ShowBrowser((char *)startPath, (char *)&argv0[0]) == true ){	//as long you keep using directories ShowBrowser will be true
						
					}
					
					char thisArgv[3][MAX_TGDSFILENAME_LENGTH];
					memset(thisArgv, 0, sizeof(thisArgv));
					strcpy(&thisArgv[0][0], curChosenBrowseFile);	//Arg0:	NDS Binary loaded
					strcpy(&thisArgv[1][0], argv0);					//Arg1: ARGV0
					addARGV(2, (char*)&thisArgv);
					break;
				}
				else if(keysPressed()&KEY_B){
					
					char thisArgv[3][MAX_TGDSFILENAME_LENGTH];
					memset(thisArgv, 0, sizeof(thisArgv));
					strcpy(&thisArgv[0][0], "");	//Arg0:	
					strcpy(&thisArgv[1][0], "");	//Arg1: 
					strcpy(&thisArgv[2][0], "");	//Arg2:
					addARGV(3, (char*)&thisArgv);
					break;
				}
			}
			
			//Compressed .NDS file? Decompress it first
			char tmpName[256+1];
			char ext[256+1];
			strcpy(tmpName, curChosenBrowseFile);
			separateExtension(tmpName, ext);
			strlwr(ext);
			if(strncmp(ext,".zip", 4) == 0){
				char unzippedFile[MAX_TGDSFILENAME_LENGTH+1];
				load_gz((char*)curChosenBrowseFile, (char*)unzippedFile);
				strcpy(curChosenBrowseFile, unzippedFile);
			}
			
			fillNDSLoaderContext(curChosenBrowseFile);
		}
		
	}
}

