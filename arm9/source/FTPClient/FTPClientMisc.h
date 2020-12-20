
#include <stdlib.h>
#include <stdio.h>
#include "FTPClientLib.h"

#include "fatfslayerTGDS.h"


typedef struct fileinfo {

	char* filename;
	//will we need a pointer to the entire path?
	//char* fsize;
	FILE* fpointer;
	int namesize;
	long int filesize;
	bool isdir;
} fileinfo;


#ifdef __cplusplus
extern "C"{
#endif

extern void free_fileinfo(fileinfo** flist);
extern fileinfo** get_dir(char* path);
extern fileinfo** get_remote_dir(char *path, struct NetBuf *nControl);
extern fileinfo** localDir;
extern fileinfo** remoteDir;
extern char* curLocal;
extern char* curRemote;

#ifdef __cplusplus
}
#endif
