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
#include "busTGDS.h"
#include "dmaTGDS.h"
#include "spifwTGDS.h"
#include "arm9.h"
#include "dmaTGDS.h"
#include "utilsTGDS.h"

//This payload has all the ARM9 core hardware, TGDS Services, so SWI/SVC can work here.
__attribute__((optimize("O0")))
__attribute__ ((noinline))
int main(int _argc, sint8 **_argv) {
	
	u32 * dest = 0x02000000;
	int arm9_payload_addr = (int)dest[0];
	int arm9_size = (int)dest[1];
	u32 * ewramDLDISrc = (u32*)dest[2];
	char * dldiMagicStr = (u32*)dest[3];
	int dldiMagicStrLen = sizeof(dldiMagicStr);
	
	//Read DLDI section into temp buffer
	//u32 * vramDLDISrc = (u32*)((u8*)0x06820000 - (16*1024));
	//memcpy(vramDLDISrc, ewramDLDISrc, (int)(16*1024));
	//coherent_user_range((u32)vramDLDISrc, (int)(16*1024));
	
	//char * vramDldiMagicStr = ((u8*)0x06820000 - (16*1024) - 48);
	//memcpy(vramDldiMagicStr, dldiMagicStr, (int)dldiMagicStrLen);
	
	//Copy ARGV
	memcpy(((u8*)0x06820000 - (16*1024) - 48 - 0x200), (u32*)0x02FFFE70, (int)0x200);
	
	u32 arm9_payload_addr_phys = (u32)0x02300000;
	memcpy((u32*)arm9_payload_addr_phys, (u32*)arm9_payload_addr, arm9_size);
	
	//Restore ARGV
	memcpy(((u8*)0x02FFFE70), ((u8*)0x06820000 - (16*1024) - 48 - 0x200), (int)0x200);
	
	u32 newarm9 = arm9_payload_addr_phys - 512;	
	
	//Read flash mem to detect NDS hardware by firmware.
	u32 * flashMem = (u32 *)newarm9;
	ReadFirmwareARM7Ext(flashMem);
	struct sIPCSharedTGDS * TGDSIPC = TGDSIPCStartAddress;
	coherent_user_range((u8*)&TGDSIPC->DSFWHEADERInst.stub[0], (u8*)&TGDSIPC->DSFWHEADERInst.stub[0] + 512);
	memcpy((u8*)&TGDSIPC->DSFWHEADERInst.stub[0], (u8*)flashMem, sizeof(TGDSIPC->DSFWHEADERInst.stub));
	
	typedef void (*t_bootAddr)();
	t_bootAddr bootARM9Payload = (t_bootAddr)arm9_payload_addr_phys;
	bootARM9Payload();
	
	return 0;
}
