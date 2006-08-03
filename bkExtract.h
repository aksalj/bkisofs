#ifndef bkExtract_h
#define bkExtract_h

int extractDir(int image, Dir* tree, Path* srcDir, char* destDir,
                                                        bool keepPermissions);
int extractFile(int image, Dir* tree, FilePath* pathAndName, char* destDir,
                                                        bool keepPermissions);

#endif
