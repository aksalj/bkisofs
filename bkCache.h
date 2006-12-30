#ifndef bkcache_h
#define bkcache_h

int wcFlush(VolInfo* volInfo);
void wcInit(VolInfo* volInfo);
int wcSeekForward(VolInfo* volInfo, off_t numBytes);
int wcSeekSet(VolInfo* volInfo, off_t position);
off_t wcSeekTell(VolInfo* volInfo);
int wcWrite(VolInfo* volInfo, const char* block, off_t numBytes);

#endif
