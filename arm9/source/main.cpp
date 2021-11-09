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
#include "typedefsTGDS.h"
#include "dsregs.h"
#include "dswnifi_lib.h"
#include "keypadTGDS.h"
#include "TGDSLogoLZSSCompressed.h"
#include "fileBrowse.h"	//generic template functions from TGDS: maintain 1 source, whose changes are globally accepted by all TGDS Projects.
#include "biosTGDS.h"
#include "ipcfifoTGDSUser.h"
#include "dldi.h"
#include "global_settings.h"
#include "posixHandleTGDS.h"
#include "TGDSMemoryAllocator.h"
#include "consoleTGDS.h"
#include "soundTGDS.h"
#include "nds_cp15_misc.h"
#include "fatfslayerTGDS.h"
#include "utilsTGDS.h"
#include "click_raw.h"
#include "ima_adpcm.h"
#include "conf.h"

// Includes
#include "WoopsiTemplate.h"

//#include "tgds_multiboot_payload.h" 
#include "dmaTGDS.h"
#include "nds_cp15_misc.h"
#include "fatfslayerTGDS.h"

//TGDS Soundstreaming API
int internalCodecType = SRC_NONE; //Returns current sound stream format: WAV, ADPCM or NONE
struct fd * _FileHandleVideo = NULL; 
struct fd * _FileHandleAudio = NULL;

bool stopSoundStreamUser(){
	return stopSoundStream(_FileHandleVideo, _FileHandleAudio, &internalCodecType);
}

void closeSoundUser(){
	//Stubbed. Gets called when closing an audiostream of a custom audio decoder
}

static inline void menuShow(){
	clrscr();
	printf("     ");
	printf("     ");
	printf("ToolchainGenericDS-OnlineApp: ");
	printf("(Select): This menu. ");
	printf("(Start): FileBrowser : (A) Play WAV/IMA-ADPCM (Intel) strm ");
	printf("(D-PAD:UP/DOWN): Volume + / - ");
	printf("(D-PAD:LEFT): GDB Debugging. >%d", TGDSPrintfColor_Green);
	printf("(D-PAD:RIGHT): Demo Sound. >%d", TGDSPrintfColor_Yellow);
	printf("(B): Stop WAV/IMA-ADPCM file. ");
	printf("Current Volume: %d", (int)getVolume());
	if(internalCodecType == SRC_WAVADPCM){
		printf("ADPCM Play: >%d", TGDSPrintfColor_Red);
	}
	else if(internalCodecType == SRC_WAV){	
		printf("WAVPCM Play: >%d", TGDSPrintfColor_Green);
	}
	else{
		printf("Player Inactive");
	}
	printf("Available heap memory: %d >%d", getMaxRam(), TGDSPrintfColor_Cyan);
}

//Untar code
char args[8][MAX_TGDSFILENAME_LENGTH];
char *argvs[8];

int GUI_getConfigInt(sint8 *objname, sint8 *field, int val)
{
	sint8 name[32];
	strcpy(name, "GUI::");
	strcat(name, objname);
	return get_config_int(name, field, val);
}

//ToolchainGenericDS-LinkedModule User implementation: Called if TGDS-LinkedModule fails to reload ARM9.bin from DLDI.
int TGDSProjectReturnFromLinkedModule() {
	return -1;
}

#if (defined(__GNUC__) && !defined(__clang__))
__attribute__((optimize("O0")))
#endif

#if (!defined(__GNUC__) && defined(__clang__))
__attribute__ ((optnone))
#endif
int main(int argc, char **argv) {
	
	/*			TGDS 1.6 Standard ARM9 Init code start	*/
	bool isTGDSCustomConsole = false;	//set default console or custom console: default console
	GUI_init(isTGDSCustomConsole);
	GUI_clear();
	
	printf("              ");
	printf("              ");
	
	//xmalloc init removes args, so save them
	int i = 0;
	for(i = 0; i < argc; i++){
		argvs[i] = argv[i];
	}

	bool isCustomTGDSMalloc = true;
	setTGDSMemoryAllocator(getProjectSpecificMemoryAllocatorSetup(TGDS_ARM7_MALLOCSTART, TGDS_ARM7_MALLOCSIZE, isCustomTGDSMalloc, TGDSDLDI_ARM7_ADDRESS));
	sint32 fwlanguage = (sint32)getLanguage();
	
	//argv destroyed here because of xmalloc init, thus restore them
	for(i = 0; i < argc; i++){
		argv[i] = argvs[i];
	}

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
	
	//Show logo
	RenderTGDSLogoMainEngine((uint8*)&TGDSLogoLZSSCompressed[0], TGDSLogoLZSSCompressed_size);
	
	// Create Woopsi UI
	WoopsiTemplate WoopsiTemplateApp;
	WoopsiTemplateProc = &WoopsiTemplateApp;
	return WoopsiTemplateApp.main(argc, argv);
	
	while(1) {
		
		handleARM9SVC();	/* Do not remove, handles TGDS services */
		IRQVBlankWait();
	}

	return 0;
}

