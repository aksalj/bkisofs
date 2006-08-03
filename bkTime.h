#ifndef bktime_h
#define bkTime_h

void epochToLongString(time_t epoch, char* longString);
void epochToShortString(time_t epoch, char* shortString);
void longStringToEpoch(char* longString, time_t* epoch);

#endif
