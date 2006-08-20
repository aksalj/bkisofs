#ifndef bkExtract_h
#define bkExtract_h

int extractDir(int image, const Dir* tree, const Path* srcDir, 
               const char* destDir, bool keepPermissions);
int extractFile(int image, const Dir* tree, const FilePath* pathAndName, 
                const char* destDir, bool keepPermissions);

#endif
