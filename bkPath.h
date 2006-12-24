#ifndef bkPath_h
#define bkPath_h

#include "bkInternal.h"

bool findDirByPath(const Path* path, BkDir* tree, BkDir** dir);
void freeDirToWriteContents(DirToWrite* dir);
void freePath(Path* path);
void freePathDirs(Path* path);
int getFilenameFromPath(const char* srcPathAndName, char* filename);
int getLastDirFromString(const char* srcPath, char* dirName);
int makeFilePathFromString(const char* srcFile, FilePath* pathPath);
int makeLongerPath(const Path* origPath, const char* newDir, Path** newPath);
int makePathFromString(const char* strPath, Path* pathPath);
bool nameIsValid(const char* name);
void printDirToWrite(DirToWrite* dir, int level, int filenameTypes);

bool findDirByNewPath(const NewPath* path, BkDir* tree, BkDir** dir);
void freePathContents(NewPath* path);
int getLastNameFromPath(const char* srcPathAndName, char* lastName);
int makeNewPathFromString(const char* strPath, NewPath* pathPath);

#endif
