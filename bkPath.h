#ifndef bkPath_h
#define bkPath_h

void freePath(Path* path);
void freePathDirs(Path* path);
int getFilenameFromPath(const char* srcPathAndName, char* filename);
int getLastDirFromString(const char* srcPath, char* dirName);
int makeFilePathFromString(const char* srcFile, FilePath* pathPath);
int makeLongerPath(const Path* origPath, const char* newDir, Path** newPath);
int makePathFromString(const char* strPath, Path* pathPath);

#endif
