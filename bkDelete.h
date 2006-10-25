#ifndef bkDelete_h
#define bkDelete_h

#include "bkInternal.h"

int deleteDir(VolInfo* volInfo, Dir* tree, const Path* dirToDelete);
void deleteDirContents(VolInfo* volInfo, Dir* dir);
int deleteFile(VolInfo* volInfo, Dir* tree, const FilePath* pathAndName);

#endif
