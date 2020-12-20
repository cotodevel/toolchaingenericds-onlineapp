
#include "FTPClientMisc.h"
#include "fatfslayerTGDS.h"
#include "typedefsTGDS.h"
#include "dsregs.h"
#include "dsregs_asm.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/dir.h>
#include <unistd.h>
#include "fileBrowse.h"	//generic template functions from TGDS: maintain 1 source, whose changes are globally accepted by all TGDS Projects.

/* Takes a list of fileinfo structs and frees all the space
*   Has no return value, as free has no return value
*/

void free_fileinfo(fileinfo** flist){
	int i = 0;
	
	while(flist[i]){
		free(flist[i]->filename);
		free(flist[i]);	
		i++;
	}
	if(flist)
	{
		free(flist);
	}
}


//libfat version
/*
// Takes a path,"/" for the root
//   Returns an array of pointers to fileinfo structs with info about the local directory
//
fileinfo** get_dir(char* path){
	fileinfo** dlist;
	struct stat st;
	DIR_ITER* dir;
	int i = 0;
	int cursize = 256;
	char filename[256+1];
	dlist = (fileinfo **)calloc(cursize, sizeof(fileinfo*));
	dir = diropen (path); 

	if (dir == NULL) {
		printf ("Unable to open the directory.");
		return 0;
	} else {
	while (dirnext(dir, filename, &st) == 0) {
		if(i >= cursize - 3){
			cursize *= 2;
			dlist = realloc(dlist,cursize);
		}
		dlist[i] = malloc(sizeof(fileinfo));
		dlist[i]->filesize = st.st_size;
		dlist[i]->namesize = strlen(filename);
		dlist[i]->filename = calloc(1,sizeof(char)*dlist[i]->namesize + 1);
		strncpy(dlist[i]->filename,filename,dlist[i]->namesize);
		dlist[i]->fpointer = 0;
		if (st.st_mode & S_IFDIR)
			dlist[i]->isdir = 1;
		else dlist[i]->isdir = 0;
		//if(dlist[i]->isdir == 1) dlist[i]->filename[dlist[i]->namesize] = '/'; 
		dlist[i]->filename[dlist[i]->namesize] = 0; 
		i++;
		// st.st_mode & S_IFDIR indicates a directory
		//printf ("%s: %s\n", (st.st_mode & S_IFDIR ? " DIR" : "FILE"), filename);
	}
	dirclose (dir);
	}
	return dlist;
}

// Takes a path, NULL for the current directory, and a struct NetBuf struct
//   Returns an array of fileinfo pointers containing information about the remote listing
//
fileinfo** get_remote_dir(char *path, struct NetBuf *nControl){
	fileinfo** dlist;
	//struct stat st;
	DIR_ITER* dir;
	int i = 0;
	int cursize = 1024;
	char filename[1024];
	char permissions[11];
	char* stringdir;
	char* offset;
	long int _filesize;
	
	dlist = (fileinfo **)calloc(cursize, sizeof(fileinfo*));
	stringdir = FtpDir(NULL,path,nControl);
	offset = strtok(stringdir,"\n");

    while(offset) {
       sscanf(offset, "%s%*s%*s%*s%ld%*s%*s%*s %255[^\n]", permissions, &_filesize, filename);
	   	if(i >= cursize - 3){
			cursize *= 2;
			dlist = realloc(dlist,cursize);
		}
	   	dlist[i] = malloc(sizeof(fileinfo));
		dlist[i]->namesize = strlen(filename);
		dlist[i]->filesize = _filesize;
		dlist[i]->filename = calloc(1,sizeof(char)*dlist[i]->namesize + 2);
		strncpy(dlist[i]->filename,filename,dlist[i]->namesize);
		
		if(permissions[0] == 'd') dlist[i]->isdir = 1;
		else dlist[i]->isdir = 0;
		
		dlist[i]->filename[dlist[i]->namesize] = 0;
		i++;
       offset = strtok(NULL, "\n");
    }
	return dlist;
}
*/

fileinfo** localDir;
fileinfo** remoteDir;
char* curLocal;
char* curRemote;

//TGDS Version
// Takes a path,"/" for the root
//   Returns an array of pointers to fileinfo structs with info about the local directory
//
fileinfo** get_dir(char* path){
	fileinfo** dlist;
	struct FileClassList * menuIteratorfileClassListCtx = NULL;
	
	int i = 0;
	int cursize = 256;
	char filename[256+1];
	dlist = (fileinfo **)calloc(cursize, sizeof(fileinfo*));
	
	//Init TGDS FS Directory Iterator Context(s). Mandatory to init them like this!! Otherwise several functions won't work correctly.
	menuIteratorfileClassListCtx = initFileList();
	cleanFileList(menuIteratorfileClassListCtx);
	
	int startFromIndex = 0;
	struct FileClass * fileClassInst = NULL;
	fileClassInst = FAT_FindFirstFile(path, menuIteratorfileClassListCtx, startFromIndex);
	while(fileClassInst != NULL){
		if(i >= cursize - 3){
			cursize *= 2;
			dlist = realloc(dlist,cursize);
		}
		//directory?
		if(fileClassInst->type == FT_DIR){
			char tmpBuf[MAX_TGDSFILENAME_LENGTH+1];
			strcpy(tmpBuf, fileClassInst->fd_namefullPath);
			parseDirNameTGDS(tmpBuf);
			strcpy(fileClassInst->fd_namefullPath, tmpBuf);
			dlist[i]->isdir = 1;
		}
		//file?
		else if(fileClassInst->type  == FT_FILE){
			char tmpBuf[MAX_TGDSFILENAME_LENGTH+1];
			strcpy(tmpBuf, fileClassInst->fd_namefullPath);
			parsefileNameTGDS(tmpBuf);
			strcpy(fileClassInst->fd_namefullPath, tmpBuf);
			dlist[i]->isdir = 0;
		}
		dlist[i] = malloc(sizeof(fileinfo));
		dlist[i]->filesize = FS_getFileSize((char*)fileClassInst->fd_namefullPath);
		dlist[i]->namesize = strlen(fileClassInst->fd_namefullPath);
		dlist[i]->filename = calloc(1,sizeof(char)*dlist[i]->namesize + 1);
		strncpy(dlist[i]->filename, fileClassInst->fd_namefullPath, dlist[i]->namesize);
		dlist[i]->fpointer = 0;
		dlist[i]->filename[dlist[i]->namesize] = 0; 
		i++;
		
		//more file/dir objects?
		fileClassInst = FAT_FindNextFile(path, menuIteratorfileClassListCtx);
	}
	
	//Free TGDS Dir API context
	freeFileList(menuIteratorfileClassListCtx);
	return dlist;
}

// Takes a path, NULL for the current directory, and a struct NetBuf struct
//   Returns an array of fileinfo pointers containing information about the remote listing
//
fileinfo** get_remote_dir(char *path, struct NetBuf *nControl){
	fileinfo** dlist;
	int i = 0;
	int cursize = 1024;
	char filename[1024];
	char permissions[11];
	char* stringdir;
	char* offset;
	long int _filesize;
	
	dlist = (fileinfo **)calloc(cursize, sizeof(fileinfo*));
	stringdir = FtpDir(NULL,path,nControl);
	offset = strtok(stringdir,"\n");

    while(offset) {
       sscanf(offset, "%s%*s%*s%*s%ld%*s%*s%*s %255[^\n]", permissions, &_filesize, filename);
	   	if(i >= cursize - 3){
			cursize *= 2;
			dlist = realloc(dlist,cursize);
		}
	   	dlist[i] = malloc(sizeof(fileinfo));
		dlist[i]->namesize = strlen(filename);
		dlist[i]->filesize = _filesize;
		dlist[i]->filename = calloc(1,sizeof(char)*dlist[i]->namesize + 2);
		strncpy(dlist[i]->filename,filename,dlist[i]->namesize);
		
		if(permissions[0] == 'd') dlist[i]->isdir = 1;
		else dlist[i]->isdir = 0;
		
		dlist[i]->filename[dlist[i]->namesize] = 0;
		i++;
       offset = strtok(NULL, "\n");
    }
	return dlist;
}