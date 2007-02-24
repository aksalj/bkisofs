int addToHardLinkTable(VolInfo* volInfo, off_t position, ino_t inode);
BkHardLink* findInHardLinkTable(VolInfo* volInfo, off_t position, ino_t inode);
