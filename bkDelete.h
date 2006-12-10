#ifndef bkDelete_h
#define bkDelete_h

#include "bkInternal.h"

int deleteDir(VolInfo* volInfo, BkDir* tree, const Path* dirToDelete);
void deleteDirContents(VolInfo* volInfo, BkDir* dir);
int deleteFile(VolInfo* volInfo, BkDir* tree, const FilePath* pathAndName);

#endif
