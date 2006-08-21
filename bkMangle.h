#ifndef bkMangle_h
#define bkMangle_h

bool charIsValid9660(char theChar);
unsigned hashString(const char *str, unsigned int length);
int mangleDir(const Dir* origDir, DirToWrite* newDir, int filenameTypes);
void mangleNameFor9660(char* origName, char* newName, bool isADir);

#endif
