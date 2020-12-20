
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

//TGDS required version: IPC Version: 1.3

//IPC FIFO Description: 
//		struct sIPCSharedTGDS * TGDSIPC = TGDSIPCStartAddress; 														// Access to TGDS internal IPC FIFO structure. 		(ipcfifoTGDS.h)
//		struct sIPCSharedTGDSSpecific * TGDSUSERIPC = (struct sIPCSharedTGDSSpecific *)TGDSIPCUserStartAddress;		// Access to TGDS Project (User) IPC FIFO structure	(ipcfifoTGDSUser.h)

#include "ipcfifoTGDS.h"
#include "ipcfifoTGDSUser.h"
#include "dsregs.h"
#include "dsregs_asm.h"
#include "InterruptsARMCores_h.h"
#include "biosTGDS.h"
#include "loader.h"
#include "dmaTGDS.h"

#ifdef ARM7
#include <string.h>

#include "main.h"
#include "wifi_arm7.h"
#include "spifwTGDS.h"

#endif

#ifdef ARM9

#include <stdbool.h>
#include "main.h"
#include "wifi_arm9.h"
#include "nds_cp15_misc.h"
#include "dldi.h"

#endif

#ifdef ARM9
__attribute__((section(".itcm")))
#endif
struct sIPCSharedTGDSSpecific* getsIPCSharedTGDSSpecific(){
	struct sIPCSharedTGDSSpecific* sIPCSharedTGDSSpecificInst = (__attribute__((packed)) struct sIPCSharedTGDSSpecific*)(TGDSIPCUserStartAddress);
	return sIPCSharedTGDSSpecificInst;
}

#ifdef ARM9
__attribute__((section(".itcm")))
#endif
void HandleFifoNotEmptyWeakRef(uint32 cmd1,uint32 cmd2){
	
	switch (cmd1) {
		//NDS7: 
		#ifdef ARM7
		case(ARM7COMMAND_RELOADNDS):{
			runBootstrapARM7();	//ARM7 Side
		}
		break;
		#endif
		
		//NDS9: 
		#ifdef ARM9
		
		case(0x11ff00ff):{
			/*
			clrscr();
			printf("----");
			printf("----");
			printf("----");
			printf("----");
			printf("----");
			printf("----");
			printf("----");
			printf("----");
			printf("----");
			printf("----");
			printf("----");
			*/
			printf("DLDI FAIL @ ARM7");
		}
		break;
		
		case(0x22ff11ff):{
			/*
			clrscr();
			printf("----");
			printf("----");
			printf("----");
			printf("----");
			printf("----");
			printf("----");
			printf("----");
			printf("----");
			printf("----");
			printf("----");
			printf("----");
			*/
			printf("DLDI OK @ ARM7");
		}
		break;
		
		
		case(0xff11ff22):{
			/*
			clrscr();
			printf("----");
			printf("----");
			printf("----");
			printf("----");
			printf("----");
			printf("----");
			printf("----");
			printf("----");
			printf("----");
			printf("----");
			printf("----");
			*/
			printf("ARM7 Reloading... please wait");
		}
		break;
		
		case(0xff11ff44):{
			/*
			clrscr();
			printf("----");
			printf("----");
			printf("----");
			printf("----");
			printf("----");
			printf("----");
			printf("----");
			printf("----");
			printf("----");
			printf("----");
			printf("----");
			*/
			printf("ARM7 ALIVE!");
		}
		break;
		case(0xff33ff55):{
			/*
			clrscr();
			printf("----");
			printf("----");
			printf("----");
			printf("----");
			printf("----");
			printf("----");
			printf("----");
			printf("----");
			printf("----");
			printf("----");
			printf("----");
			*/
			printf("ARM7 RELOAD SECTION OK!");
		}
		break;
		
		case(NDSLOADER_ENTERGDB_FROM_ARM7):{
			EWRAMPrioToARM9();
			GDBEnabled = true;
		}
		break;
		
		#endif
		
		
		//shared
		case(NDSLOADER_INITDLDIARM7_BUSY):{
			#ifdef ARM9
			coherent_user_range_by_size((u32)&_io_dldi_stub, (int)16*1024);	//prevent cache problems
			memcpy((u32*)NDS_LOADER_DLDISECTION_UNCACHED, (u32*)&_io_dldi_stub, (int)16*1024);
			setNDSLoaderInitStatus(NDSLOADER_INITDLDIARM7_DONE);
			#endif
		}
		break;
	}
	
}

#ifdef ARM9
__attribute__((section(".itcm")))
#endif
void HandleFifoEmptyWeakRef(uint32 cmd1,uint32 cmd2){
}

//project specific stuff

void EWRAMPrioToARM7(){
	//give EWRAM to ARM7
	*(u16*)0x04000204 = (  (*(u16*)0x04000204 & ~(1<<15)) | (1<<15));
}

void EWRAMPrioToARM9(){
	//give EWRAM to ARM9
	*(u16*)0x04000204 = (  (*(u16*)0x04000204 & ~(1<<15)) | (0<<15));
}