# CC, AR, RM defined in parent makefile

OBJECTS = bkRead7x.o bkAdd.o bkDelete.o bkExtract.o bkRead.o bkPath.o bkMangle.o bkWrite.o bkWrite7x.o bkTime.o bkSort.o bkError.o bkGet.o bkSet.o
GLOBALDEPS = Makefile bk.h bkInternal.h
# the _FILE_OFFSET_BITS=64 is to enable stat() for large files
# DEBUG and -g only used during development
GLOBALFLAGS = -D_FILE_OFFSET_BITS=64 -Wall -pedantic -std=c99

bk.a: $(OBJECTS)
	$(AR) -cr bk.a $(OBJECTS)

bkRead7x.o: bkRead7x.c bkRead7x.h $(GLOBALDEPS)
	
bkAdd.o: bkAdd.c bkAdd.h $(GLOBALDEPS)
	$(CC) bkAdd.c $(GLOBALFLAGS) -c
bkDelete.o: bkDelete.c bkDelete.h $(GLOBALDEPS)
	$(CC) bkDelete.c $(GLOBALFLAGS) -c
bkExtract.o: bkExtract.c bkExtract.h $(GLOBALDEPS)
	$(CC) bkExtract.c $(GLOBALFLAGS) -c
bkRead.o: bkRead.c bkRead.h $(GLOBALDEPS)
	$(CC) bkRead.c $(GLOBALFLAGS) -c
bkPath.o: bkPath.c bkPath.h $(GLOBALDEPS)
	$(CC) bkPath.c $(GLOBALFLAGS) -c
bkMangle.o: bkMangle.c bkMangle.h $(GLOBALDEPS)
	$(CC) bkMangle.c $(GLOBALFLAGS) -c
bkWrite.o: bkWrite.c bkWrite.h $(GLOBALDEPS)
	$(CC) bkWrite.c $(GLOBALFLAGS) -c
bkWrite7x.o: bkWrite7x.c bkWrite7x.h $(GLOBALDEPS)
	$(CC) bkWrite7x.c $(GLOBALFLAGS) -c
bkTime.o: bkTime.c bkTime.h $(GLOBALDEPS)
	$(CC) bkTime.c $(GLOBALFLAGS) -c
bkSort.o: bkSort.c bkSort.h $(GLOBALDEPS)
	$(CC) bkSort.c $(GLOBALFLAGS) -c
bkError.o: bkError.c bkError.h $(GLOBALDEPS)
	$(CC) bkError.c $(GLOBALFLAGS) -c
bkGet.o: bkGet.c bkGet.h $(GLOBALDEPS)
	$(CC) bkGet.c $(GLOBALFLAGS) -c
bkSet.o: bkSet.c bkSet.h $(GLOBALDEPS)
	$(CC) bkSet.c $(GLOBALFLAGS) -c

clean: 
	$(RM) *.o bk.a
