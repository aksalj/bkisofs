void bkClose(int file);
int bkFstat(int file, BkStatStruct* statStruct);
size_t bkRead(int file, void* dest, size_t numBytes);
bk_off_t bkSeekSet(int file, bk_off_t offset, int origin);
bk_off_t bkSeekTell(int file);
int bkStat(const char* pathAndName, BkStatStruct* statStruct);
size_t bkWrite(int file, const void* src, size_t numBytes);
size_t readRead(VolInfo* volInfo, void* dest, size_t numBytes);
bk_off_t readSeekSet(VolInfo* volInfo, bk_off_t offset, int origin);
bk_off_t readSeekTell(VolInfo* volInfo);
