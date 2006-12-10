#ifndef bkAdd_h
#define bkAdd_h

int addDir(VolInfo* volInfo, BkDir* tree, const char* srcPath, 
           const Path* destDir);
int addFile(BkDir* tree, const char* srcPathAndName, const Path* destDir);
bool itemIsInDir(const char* name, const BkDir* dir);

#endif
