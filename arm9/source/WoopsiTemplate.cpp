// Includes
#include "WoopsiTemplate.h"
#include "woopsiheaders.h"
#include "bitmapwrapper.h"
#include "bitmap.h"
#include "graphics.h"
#include "rect.h"
#include "gadgetstyle.h"
#include "fonts/newtopaz.h"
#include "woopsistring.h"
#include "colourpicker.h"
#include "filerequester.h"
#include "soundTGDS.h"
#include "main.h"
#include "posixHandleTGDS.h"
#include "keypadTGDS.h"
#include "dswnifi_lib.h"
#include "conf.h"
#include "ftpMisc.h"
#include "ftpServer.h"
#include "FTPClientLib.h"
#include "FTPClientMisc.h"
#include "fileBrowse.h"	//generic template functions from TGDS: maintain 1 source, whose changes are globally accepted by all TGDS Projects.
#include "xenofunzip.h"
#include "cartHeader.h"
#include "ipcfifoTGDSUser.h"
#include "loader.h"

__attribute__((section(".dtcm")))
WoopsiTemplate * WoopsiTemplateProc = NULL;

void WoopsiTemplate::startup(int argc, char **argv) {
	
	Rect rect;

	/** SuperBitmap preparation **/
	// Create bitmap for superbitmap
	Bitmap* superBitmapBitmap = new Bitmap(164, 191);

	// Get a graphics object from the bitmap so that we can modify it
	Graphics* gfx = superBitmapBitmap->newGraphics();

	// Clean up
	delete gfx;

	// Create screens
	AmigaScreen* newScreen = new AmigaScreen(TGDSPROJECTNAME, Gadget::GADGET_DRAGGABLE, AmigaScreen::AMIGA_SCREEN_SHOW_DEPTH);
	woopsiApplication->addGadget(newScreen);
	newScreen->setPermeable(true);

	// Add child windows
	AmigaWindow* controlWindow = new AmigaWindow(0, 13, 256, 33, "Controls", Gadget::GADGET_DRAGGABLE, AmigaWindow::AMIGA_WINDOW_SHOW_DEPTH);
	newScreen->addGadget(controlWindow);

	// Controls
	controlWindow->getClientRect(rect);

	_Index = new Button(rect.x, rect.y, 41, 16, "Index");	//_Index->disable();
	_Index->setRefcon(2);
	controlWindow->addGadget(_Index);
	_Index->addGadgetEventHandler(this);
	
	_lastFile = new Button(rect.x + 41, rect.y, 17, 16, "<");
	_lastFile->setRefcon(3);
	controlWindow->addGadget(_lastFile);
	_lastFile->addGadgetEventHandler(this);
	
	_nextFile = new Button(rect.x + 41 + 17, rect.y, 17, 16, ">");
	_nextFile->setRefcon(4);
	controlWindow->addGadget(_nextFile);
	_nextFile->addGadgetEventHandler(this);
	
	_play = new Button(rect.x + 41 + 17 + 17, rect.y, 40, 16, "Play");
	_play->setRefcon(5);
	controlWindow->addGadget(_play);
	_play->addGadgetEventHandler(this);
	
	_stop = new Button(rect.x + 41 + 17 + 17 + 40, rect.y, 40, 16, "Stop");
	_stop->setRefcon(6);
	controlWindow->addGadget(_stop);
	_stop->addGadgetEventHandler(this);
	
	_upVolume = new Button(rect.x + 41 + 17 + 17 + 40 + 40, rect.y, 40, 16, "Vol.+");
	_upVolume->setRefcon(7);
	controlWindow->addGadget(_upVolume);
	_upVolume->addGadgetEventHandler(this);
	
	_downVolume = new Button(rect.x + 41 + 17 + 17 + 40 + 40 + 40, rect.y, 40, 16, "Vol.-");
	_downVolume->setRefcon(8);
	controlWindow->addGadget(_downVolume);
	_downVolume->addGadgetEventHandler(this);
	
	// Add File listing screen
	_fileScreen = new AmigaScreen("File List", Gadget::GADGET_DRAGGABLE, AmigaScreen::AMIGA_SCREEN_SHOW_DEPTH | AmigaScreen::AMIGA_SCREEN_SHOW_FLIP);
	woopsiApplication->addGadget(_fileScreen);
	_fileScreen->setPermeable(true);
	_fileScreen->flipToTopScreen();
	// Add screen background
	_fileScreen->insertGadget(new Gradient(0, SCREEN_TITLE_HEIGHT, 256, 192 - SCREEN_TITLE_HEIGHT, woopsiRGB(0, 31, 0), woopsiRGB(0, 0, 31)));
	
	// Create FileRequester
	_fileReq = new FileRequester(10, 10, 150, 150, "Files", "/", GADGET_DRAGGABLE | GADGET_DOUBLE_CLICKABLE);
	_fileReq->setRefcon(1);
	newScreen->addGadget(_fileReq);
	_fileReq->addGadgetEventHandler(this);
	currentFileRequesterIndex = 0;
	
	// Create listboxes
	remoteFiles = new ScrollingListBox(40, 15, 215, 170);
	remoteFiles->setRefcon(9);
	newScreen->addGadget(remoteFiles);
	remoteFiles->addGadgetEventHandler(this);
	
	_MultiLineTextBoxLogger = NULL;	//destroyable TextBox
	
	enableDrawing();	// Ensure Woopsi can now draw itself
	redraw();			// Draw initial state
	
	//FTP init must go here once it works on real hardware
	bool isFTPServer = false;
	ftpInit(isFTPServer);
}

void WoopsiTemplate::shutdown() {
	Woopsi::shutdown();
}

void WoopsiTemplate::waitForAOrTouchScreenButtonMessage(MultiLineTextBox* thisLineTextBox, const WoopsiString& thisText){
	thisLineTextBox->appendText(thisText);
	scanKeys();
	while((!(keysDown() & KEY_A)) && (!(keysDown() & KEY_TOUCH))){
		scanKeys();
	}
	scanKeys();
	while((keysDown() & KEY_A) && (keysDown() & KEY_TOUCH)){
		scanKeys();
	}
}

void WoopsiTemplate::handleValueChangeEvent(const GadgetEventArgs& e) {

	// Did a gadget fire this event?
	if (e.getSource() != NULL) {
	
		// Is the gadget the file requester?
		if ((e.getSource()->getRefcon() == 1) && (((FileRequester*)e.getSource())->getSelectedOption() != NULL)) {
			
			//Play WAV/ADPCM if selected from the FileRequester
			WoopsiString strObj = ((FileRequester*)e.getSource())->getSelectedOption()->getText();
			memset(currentFileChosen, 0, sizeof(currentFileChosen));
			strObj.copyToCharArray(currentFileChosen);
			
			//Boot .NDS file! (homebrew only)
			char tmpName[256];
			char ext[256];
			strcpy(tmpName, currentFileChosen);
			separateExtension(tmpName, ext);
			strlwr(ext);
			if(strncmp(ext,".nds", 4) == 0){
				char thisArgv[3][MAX_TGDSFILENAME_LENGTH];
				memset(thisArgv, 0, sizeof(thisArgv));
				strcpy(&thisArgv[0][0], TGDSPROJECTNAME);	//Arg0:	This Binary loaded
				strcpy(&thisArgv[1][0], currentFileChosen);	//Arg1:	NDS Binary reloaded
				strcpy(&thisArgv[2][0], "");					//Arg2: NDS Binary ARG0
				u32 * payload = getTGDSMBV3ARM7Bootloader();
				TGDSMultibootRunNDSPayload(currentFileChosen, (u8*)payload, 3, (char*)&thisArgv);
			}
			
			pendPlay = 1;

			/* 
			//Destroyable Textbox implementation init
			Rect rect;
			_fileScreen->getClientRect(rect);
			_MultiLineTextBoxLogger = new MultiLineTextBox(rect.x, rect.y, 262, 170, "Loading\n...", Gadget::GADGET_DRAGGABLE, 5);
			_fileScreen->addGadget(_MultiLineTextBoxLogger);
			
			_MultiLineTextBoxLogger->removeText(0);
			_MultiLineTextBoxLogger->moveCursorToPosition(0);
			_MultiLineTextBoxLogger->appendText("File open OK: ");
			_MultiLineTextBoxLogger->appendText(strObj);
			_MultiLineTextBoxLogger->appendText("\n");
			_MultiLineTextBoxLogger->appendText("Please wait calculating CRC32... \n");
			
			char arrBuild[256+1];
			sprintf(arrBuild, "%s%x\n", "Invalid file: crc32 = 0x", crc32);
			_MultiLineTextBoxLogger->appendText(WoopsiString(arrBuild));
			
			sprintf(arrBuild, "%s%x\n", "Expected: crc32 = 0x", 0x5F35977E);
			_MultiLineTextBoxLogger->appendText(WoopsiString(arrBuild));
			
			waitForAOrTouchScreenButtonMessage(_MultiLineTextBoxLogger, "Press (A) or tap touchscreen to continue. \n");
			
			_MultiLineTextBoxLogger->invalidateVisibleRectCache();
			_fileScreen->eraseGadget(_MultiLineTextBoxLogger);
			_MultiLineTextBoxLogger->destroy();	//same as delete _MultiLineTextBoxLogger;
			//Destroyable Textbox implementation end
			*/
		}
	}
}

void WoopsiTemplate::handleLidClosed() {
	// Lid has just been closed
	_lidClosed = true;

	// Run lid closed on all gadgets
	s32 i = 0;
	while (i < _gadgets.size()) {
		_gadgets[i]->lidClose();
		i++;
	}
}

void WoopsiTemplate::handleLidOpen() {
	// Lid has just been opened
	_lidClosed = false;

	// Run lid opened on all gadgets
	s32 i = 0;
	while (i < _gadgets.size()) {
		_gadgets[i]->lidOpen();
		i++;
	}
}

void WoopsiTemplate::getFileListing(fileinfo** output, ScrollingListBox* box)
{
	int i = 0;
	while(output[i])
	{
		if( output[i]->isdir )
		{
			box->addOption(output[i]->filename, i, box->getShineColour(), box->getBackColour(), box->getShineColour(), box->getHighlightColour());
		}
		else
		{
			box->addOption(output[i]->filename, i);
		}
		i++;
	}
}

#if (defined(__GNUC__) && !defined(__clang__))
__attribute__((optimize("O0")))
#endif

#if (!defined(__GNUC__) && defined(__clang__))
__attribute__ ((optnone))
#endif
void WoopsiTemplate::handleClickEvent(const GadgetEventArgs& e) {
	switch (e.getSource()->getRefcon()) {
		//_Index Event
		case 2:{
			//Get fileRequester size, if > 0, set the first element selected
			FileRequester * freqInst = _fileReq;
			FileListBox* freqListBox = freqInst->getInternalListBoxObject();
			if(freqListBox->getOptionCount() > 0){
				freqListBox->setSelectedIndex(0);
			}
			currentFileRequesterIndex = 0;
		}	
		break;
		
		//_lastFile Event
		case 3:{
			FileRequester * freqInst = _fileReq;
			FileListBox* freqListBox = freqInst->getInternalListBoxObject();
			if(currentFileRequesterIndex > 0){
				currentFileRequesterIndex--;
			}
			if(freqListBox->getOptionCount() > 0){
				freqListBox->setSelectedIndex(currentFileRequesterIndex);
			}
		}	
		break;
		
		//_nextFile Event
		case 4:{
			FileRequester * freqInst = _fileReq;
			FileListBox* freqListBox = freqInst->getInternalListBoxObject();
			if(currentFileRequesterIndex < (freqListBox->getOptionCount() - 1) ){
				currentFileRequesterIndex++;
				freqListBox->setSelectedIndex(currentFileRequesterIndex);
			}
		}	
		break;
		
		//_play Event
		case 5:{
			//Play WAV/ADPCM if selected from the FileRequester
			WoopsiString strObj = _fileReq->getSelectedOption()->getText();
			memset(currentFileChosen, 0, sizeof(currentFileChosen));
			strObj.copyToCharArray(currentFileChosen);
			pendPlay = 1;
		}	
		break;
		
		//_stop Event
		case 6:{
			pendPlay = 2;
		}	
		break;
		
		//_upVolume Event
		case 7:{
			struct touchPosition touch;
			XYReadScrPosUser(&touch);
			volumeUp(touch.px, touch.py);
			
		}	
		break;
		
		//_downVolume Event
		case 8:{
			struct touchPosition touch;
			XYReadScrPosUser(&touch);
			volumeDown(touch.px, touch.py);
		}	
		break;
		
		//remoteFiles Event
		case 9:{
			char arrBuild[256+1];
			int index = 0;
			while((index = remoteFiles->getSelectedIndex()) >= 0)
			{
				if( remoteDir[index]->isdir )
				{
					char curPth[256+1];
					char newPth[256+1];
					memset(curPth, 0, sizeof(curPth));
					memset(newPth, 0, sizeof(newPth));
					
					remoteFiles->getSelectedOption()->getText().copyToCharArray(newPth);
					
					WoopsiUI::WoopsiString curPath = (WoopsiUI::WoopsiString)WoopsiString(remoteDirPath);
					curPath.copyToCharArray(curPth);
					
					int compare = remoteFiles->getSelectedOption()->getText().compareTo("..");
					if (compare == 0){
						//Leaving dir
						leaveDir(curPth);
						WoopsiString newPath;
						newPath.setText((const char*)curPth);
						newPath.copyToCharArray(remoteDirPath);
						//FtpCDUp(conn)
					}
					else{
						// Enter a new directory
						strcat(curPth, (char*)newPth);
						WoopsiString newPath;
						newPath.setText((const char*)&curPth[0]);
						newPath.copyToCharArray(remoteDirPath);
					}
					
					if((FtpChdir(remoteDirPath, conn) == 1) && FtpPwd(remoteDirPath, 256, conn) )
					{
						//Free remoteFiles context
						remoteFiles->removeAllOptions();
						free_fileinfo(remoteDir);
						
						remoteDir = get_remote_dir(remoteDirPath, conn);
						getFileListing(remoteDir, remoteFiles);
						_MultiLineTextBoxLogger->appendText(WoopsiString("Retrieving TGDS Content: OK.\n"));
					}
					else{
						_MultiLineTextBoxLogger->appendText(WoopsiString("Retrieving TGDS Content: ERROR.\n"));
					}
				}
				else
				{
					char newFile[256+1];
					memset(newFile, 0, sizeof(newFile));
					remoteFiles->getSelectedOption()->getText().copyToCharArray(newFile);
					
					//Progress bar doesnt work right yet....
					//progress = new ProgressBar(5, 20, 200, 20);
					//fileScreen->addGadget(progress);
					//progress->setMaximumValue(remoteDir[index]->filesize %100);
					//progress->setMinimumValue(0);
					//progress->setFillColour(woopsiRGB(102, 255, 255));
					//progress->setShineColour(woopsiRGB(102, 255, 255));
					//progress->setValue(0);
					
					//_fileScreen->hide();
					//char size[1024];
					//sprintf(size, "%d", remoteDir[index]->filesize);
					
					char newFilePath[256+1];
					memset(newFilePath, 0, sizeof(newFilePath));
					sprintf(newFilePath, "%s/%s", remoteDirPath, newFile);
					
					sprintf(arrBuild, "Downloading file:%s\n", newFilePath);
					WoopsiTemplateProc->_MultiLineTextBoxLogger->appendText(WoopsiString(arrBuild));
					
					if(mkdir((const sint8 *)remoteDirPath, 777)){
						sprintf(arrBuild, "Dir :%s doesn't exists. Creating. \n", remoteDirPath);
						WoopsiTemplateProc->_MultiLineTextBoxLogger->appendText(WoopsiString(arrBuild));
					}
					else{
						sprintf(arrBuild, "Dir :%s doesn't exists. Failed Creating. \n", remoteDirPath);
						WoopsiTemplateProc->_MultiLineTextBoxLogger->appendText(WoopsiString(arrBuild));
					}
					
					if(!FtpGet(newFilePath, newFile,'I',conn)){
						sprintf(arrBuild, "Failed to get file:%s\n", newFile);
						WoopsiTemplateProc->_MultiLineTextBoxLogger->appendText(WoopsiString(arrBuild));
					}
					
					sprintf(arrBuild, "Download Complete:%s\n", newFile);
					WoopsiTemplateProc->_MultiLineTextBoxLogger->appendText(WoopsiString(arrBuild));
					
					//Handle Package
					int argCount = 2;
					char fileBuf[256+1];
					memset(fileBuf, 0, sizeof(fileBuf));
					strcpy(fileBuf, "0:");
					strcat(fileBuf, newFilePath);						
					strcpy(&args[0][0], "-l");	//Arg0
					strcpy(&args[1][0], fileBuf);	//Arg1
					strcpy(&args[2][0], "d /");	//Arg2
					
					int i = 0;
					for(i = 0; i < argCount; i++){	
						argvs[i] = (char*)&args[i][0];
					}
					
					switch_dswnifi_mode(dswifi_idlemode);
					
					extern int untgzmain(int argc,char **argv);
					if(untgzmain(argCount, argvs) == 0){
						sprintf(arrBuild, "Unpack OK:%s\n", fileBuf);
						WoopsiTemplateProc->_MultiLineTextBoxLogger->appendText(WoopsiString(arrBuild));
						//Descriptor is always at root SD path: 0:/descriptor.txt
						set_config_file("0:/descriptor.txt");
						char * baseTargetPath = get_config_string("Global", "baseTargetPath", "");
						char * mainApp = get_config_string("Global", "mainApp", "");
						int mainAppCRC32 = get_config_hex("Global", "mainAppCRC32", 0);
						int TGDSSdkCrc32 = get_config_hex("Global", "TGDSSdkCrc32", 0);
						WoopsiTemplateProc->_MultiLineTextBoxLogger->appendText(WoopsiString("Package decompressed correctly:"));
						
						//sprintf(arrBuild, "[%s][%s]\n", baseTargetPath, mainApp);
						//WoopsiTemplateProc->_MultiLineTextBoxLogger->appendText(WoopsiString(arrBuild));
						
						//sprintf(arrBuild, "TGDSSDK:CRC32:%x\n", TGDSSdkCrc32);
						//WoopsiTemplateProc->_MultiLineTextBoxLogger->appendText(WoopsiString(arrBuild));
						
						//Boot .NDS file! (homebrew only)
						char tmpName[256];
						char ext[256];
						strcpy(tmpName, mainApp);
						separateExtension(tmpName, ext);
						strlwr(ext);
						if(
						(strncmp(ext,".nds", 4) == 0)
						||
						(strncmp(ext,".srl", 4) == 0)
						){
							memset(fileBuf, 0, sizeof(fileBuf));
							strcpy(fileBuf, "0:/");
							strcat(fileBuf, baseTargetPath);
							strcat(fileBuf, mainApp);
							sprintf(arrBuild, "Boot:\n[%s]\n[CRC32:%x]\n", fileBuf, mainAppCRC32);
							WoopsiTemplateProc->_MultiLineTextBoxLogger->appendText(WoopsiString(arrBuild));
							char thisArgv[3][MAX_TGDSFILENAME_LENGTH];
							memset(thisArgv, 0, sizeof(thisArgv));
							strcpy(&thisArgv[0][0], TGDSPROJECTNAME);	//Arg0:	This Binary loaded
							strcpy(&thisArgv[1][0], fileBuf);	//Arg1:	NDS Binary reloaded
							strcpy(&thisArgv[2][0], "");					//Arg2: NDS Binary ARG0
							u32 * payload = getTGDSMBV3ARM7Bootloader();
							TGDSMultibootRunNDSPayload(fileBuf, (u8*)payload, 3, (char*)&thisArgv);
						}
						else{
							sprintf(arrBuild, "TGDS App not found:\n[%s]\n[CRC32:%x]\n", mainApp, mainAppCRC32);
							WoopsiTemplateProc->_MultiLineTextBoxLogger->appendText(WoopsiString(arrBuild));
						}	
					}
					else{
						sprintf(arrBuild, "Couldn't unpack TGDSPackage.:%s\n", newFile);
						WoopsiTemplateProc->_MultiLineTextBoxLogger->appendText(WoopsiString(arrBuild));
					}
					
					//Update local dir contents here maybe
				}
				remoteFiles->deselectOption(index);
			}
		}	
		break;
		
	}
}

__attribute__((section(".dtcm")))
u32 pendPlay = 0;

char currentFileChosen[256+1];

//Called once Woopsi events are ended: TGDS Main Loop
__attribute__((section(".itcm")))
void Woopsi::ApplicationMainLoop(){
	//Earlier.. main from Woopsi SDK.
	
	//Handle TGDS stuff...
	
	switch(FTPServerService()){		
		//FTP Server cases
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
		
		
		//FTP Client cases
		case(FTP_CLIENT_ACTIVE):{
			
		}
		break;
		
		case(FTP_CLIENT_DISCONNECTED_FROM_SERVER):{
			
		}
		break;
	}
	
	switch(pendPlay){
		case(1):{
			internalCodecType = playSoundStream(currentFileChosen, _FileHandleVideo, _FileHandleAudio, TGDS_ARM7_AUDIOBUFFER_STREAM);
			if(internalCodecType == SRC_NONE){
				//stop right now
				pendPlay = 2;
			}
			else{
				pendPlay = 0;
			}
		}
		break;
		case(2):{
			stopSoundStreamUser();
			pendPlay = 0;
		}
		break;
	}
	
	
}