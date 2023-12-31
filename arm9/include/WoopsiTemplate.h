#ifndef _DEMO_H_
#define _DEMO_H_

#include <time.h>
typedef int (*FtpCallback)(/*struct NetBuf * */ void *nControl, int xfered, void *arg);
struct NetBuf {
    char *cput,*cget;
    int handle;
    int cavail,cleft;
    char *buf;
    int dir;
    void *ctrl;	//struct NetBuf *
    void *data; //struct NetBuf *
    int cmode;
    struct timeval idletime;
    FtpCallback idlecb;
    void *idlearg;
    int xfered;
    int cbbytes;
    int xfered1;
    char response[256];
};

#ifdef __cplusplus
#include "alert.h"
#include "woopsi.h"
#include "woopsiheaders.h"
#include "filerequester.h"
#include "textbox.h"
#include "soundTGDS.h"
#include "button.h"
#include "scrollinglistbox.h"
#include "fatfslayerTGDS.h"
#include "FTPClientLib.h"

#include <string>
using namespace std;
using namespace WoopsiUI;

typedef struct fileinfo {
	char* filename;
	//will we need a pointer to the entire path?
	//char* fsize;
	FILE* fpointer;
	int namesize;
	long int filesize;
	bool isdir;
} fileinfo;

class WoopsiTemplate : public Woopsi, public GadgetEventHandler {
public:
	void startup(int argc, char **argv);
	void shutdown();
	void handleValueChangeEvent(const GadgetEventArgs& e);	//Handles UI events if they change
	void handleClickEvent(const GadgetEventArgs& e);	//Handles UI events when they take click action
	void waitForAOrTouchScreenButtonMessage(MultiLineTextBox* thisLineTextBox, const WoopsiString& thisText);
	void getFileListing(fileinfo** output, ScrollingListBox* box);
	void handleLidClosed();
	void handleLidOpen();
	void ApplicationMainLoop();
	FileRequester* _fileReq;
	int currentFileRequesterIndex;
	AmigaScreen* _fileScreen;
	MultiLineTextBox* _MultiLineTextBoxLogger;
	Button* _Index;
	Button* _lastFile;
	Button* _nextFile;
	Button* _play;
	Button* _stop;
	Button* _upVolume;
	Button* _downVolume;
	
	fileinfo** remoteDir;
	ScrollingListBox* remoteFiles;
	char remoteDirPath[256+1];
	
	struct NetBuf *conn;
	
	int handleFTPClientCommand = 0;	//1 do  ftp
	
private:
	Alert* _alert;
};
#endif

#endif

#ifdef __cplusplus
extern "C" {
#endif

extern WoopsiTemplate * WoopsiTemplateProc;
extern u32 pendPlay;
extern char currentFileChosen[256+1];
extern int readline(char *buf,int max,struct NetBuf *ctl);

#ifdef __cplusplus
}
#endif
