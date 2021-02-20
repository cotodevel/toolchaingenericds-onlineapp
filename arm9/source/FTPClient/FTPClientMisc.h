
#include <stdlib.h>
#include <stdio.h>
#include "FTPClientLib.h"
#include "fatfslayerTGDS.h"

#ifdef __cplusplus
#include "WoopsiTemplate.h"
#endif

#ifdef __cplusplus
extern "C"{
#endif

extern void free_fileinfo(fileinfo** flist);
extern fileinfo** get_dir(char* path);
extern fileinfo** get_remote_dir(char *path, struct NetBuf *nControl);
extern fileinfo** localDir;
extern char* curLocal;

#ifdef __cplusplus
}
#endif
