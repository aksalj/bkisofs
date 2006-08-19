#ifndef bkAdd_h
#define bkAdd_h

int addDir(Dir* tree, const char* srcPath, const Path* destDir);
int addFile(Dir* tree, const char* srcPathAndName, const Path* destDir);
bool itemIsInDir(const char* name, const Dir* dir);

#endif
