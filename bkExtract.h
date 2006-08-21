#ifndef bkExtract_h
#define bkExtract_h

int extractDir(int image, const Dir* tree, const Path* srcDir, 
               const char* destDir, bool keepPermissions, 
               void(*progressFunction)(void));
int extractFile(int image, const Dir* tree, const FilePath* pathAndName, 
                const char* destDir, bool keepPermissions, 
                void(*progressFunction)(void));

#endif
