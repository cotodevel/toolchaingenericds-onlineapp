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
#include "biosTGDS.h"
#include "loader.h"
#include "busTGDS.h"
#include "dmaTGDS.h"
#include "spifwTGDS.h"

static inline void enterGDBFromARM7(void){
	SendFIFOWords(NDSLOADER_ENTERGDB_FROM_ARM7, 0);
}

int main(int _argc, sint8 **_argv) {
	/*			TGDS 1.6 Standard ARM7 Init code start	*/
	installWifiFIFO();		
	/*			TGDS 1.6 Standard ARM7 Init code end	*/
	
	waitWhileNotSetStatus(NDSLOADER_LOAD_OK);	//this bootstub will proceed only when file has been loaded properly
	
	//NDS_LOADER_IPC_ARM7BIN_UNCACHED has ARM7.bin bootcode now
	int arm7BootCodeSize = NDS_LOADER_IPC_CTX_UNCACHED->bootCode7FileSize;
	u32 arm7entryaddress = NDS_LOADER_IPC_CTX_UNCACHED->arm7EntryAddress;
	dmaTransferWord(3, (uint32)NDS_LOADER_IPC_ARM7BIN_UNCACHED, (uint32)arm7entryaddress, (uint32)arm7BootCodeSize);
	
	//reload ARM7.bin
	setNDSLoaderInitStatus(NDSLOADER_START);	
	reloadARMCore(NDS_LOADER_IPC_CTX_UNCACHED->arm7EntryAddress);
	
	//enterGDBFromARM7();	//debug
    
	while (1) {
		handleARM7SVC();	/* Do not remove, handles TGDS services */
		IRQWait(IRQ_VBLANK);
	}
   
	return 0;
}
