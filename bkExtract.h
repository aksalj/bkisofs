int copyByteBlock(int src, int dest, unsigned numBytes);
int extract(VolInfo* volInfo, BkDir* parentDir, char* nameToExtract, 
            const char* destDir, bool keepPermissions, 
            void(*progressFunction)(void));
int extractDir(VolInfo* volInfo, BkDir* srcDir, const char* destDir, 
               bool keepPermissions, void(*progressFunction)(void));
int extractFile(VolInfo* volInfo, BkFile* srcFileInTree, const char* destDir, 
                bool keepPermissions, void(*progressFunction)(void));
int extractSymlink(VolInfo* volInfo, BkSymLink* srcLink, const char* destDir);
