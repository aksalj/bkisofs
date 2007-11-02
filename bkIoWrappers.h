size_t readRead(VolInfo* volInfo, void* dest, size_t numBytes);
bk_off_t readSeekSet(VolInfo* volInfo, bk_off_t offset, int origin);
bk_off_t readSeekTell(VolInfo* volInfo);
