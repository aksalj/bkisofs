#ifndef bkExtract_h
#define bkExtract_h

int extract(VolInfo* volInfo, BkDir* parentDir, char* nameToExtract, 
            const char* destDir, bool keepPermissions, 
            void(*progressFunction)(void));
int extractDir(VolInfo* volInfo, BkDir* srcDir, const char* destDir, 
               bool keepPermissions, void(*progressFunction)(void));
int extractFile(VolInfo* volInfo, BkFile* srcFileInTree, const char* destDir, 
                bool keepPermissions, void(*progressFunction)(void));

#endif
