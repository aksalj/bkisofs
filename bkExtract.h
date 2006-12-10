#ifndef bkExtract_h
#define bkExtract_h

int extractDir(VolInfo* volInfo, int image, 
               const Path* srcDir, const char* destDir, bool keepPermissions, 
               void(*progressFunction)(void));
int extractFile(VolInfo* volInfo, int image, 
                const FilePath* pathAndName, const char* destDir, 
                bool keepPermissions, void(*progressFunction)(void));

#endif
