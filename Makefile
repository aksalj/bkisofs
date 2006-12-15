OBJECTS = bkRead7x.o bkAdd.o bkDelete.o bkExtract.o bkRead.o bkPath.o bkMangle.o bkWrite.o bkWrite7x.o bkTime.o bkSort.o bkError.o bkGet.o bkSet.o
GLOBALDEPS = Makefile bk.h bkInternal.h
# the _FILE_OFFSET_BITS=64 is to enable stat() for large files
# DEBUG and -g only used during development
GLOBALFLAGS = -D_FILE_OFFSET_BITS=64 -Wall -pedantic -std=c99

bk.a: $(OBJECTS)
	ar -cr bk.a $(OBJECTS)

bkRead7x.o: bkRead7x.c bkRead7x.h $(GLOBALDEPS)
	cc bkRead7x.c $(GLOBALFLAGS) -c
bkAdd.o: bkAdd.c bkAdd.h $(GLOBALDEPS)
	cc bkAdd.c $(GLOBALFLAGS) -c
bkDelete.o: bkDelete.c bkDelete.h $(GLOBALDEPS)
	cc bkDelete.c $(GLOBALFLAGS) -c
bkExtract.o: bkExtract.c bkExtract.h $(GLOBALDEPS)
	cc bkExtract.c $(GLOBALFLAGS) -c
bkRead.o: bkRead.c bkRead.h $(GLOBALDEPS)
	cc bkRead.c $(GLOBALFLAGS) -c
bkPath.o: bkPath.c bkPath.h $(GLOBALDEPS)
	cc bkPath.c $(GLOBALFLAGS) -c
bkMangle.o: bkMangle.c bkMangle.h $(GLOBALDEPS)
	cc bkMangle.c $(GLOBALFLAGS) -c
bkWrite.o: bkWrite.c bkWrite.h $(GLOBALDEPS)
	cc bkWrite.c $(GLOBALFLAGS) -c
bkWrite7x.o: bkWrite7x.c bkWrite7x.h $(GLOBALDEPS)
	cc bkWrite7x.c $(GLOBALFLAGS) -c
bkTime.o: bkTime.c bkTime.h $(GLOBALDEPS)
	cc bkTime.c $(GLOBALFLAGS) -c
bkSort.o: bkSort.c bkSort.h $(GLOBALDEPS)
	cc bkSort.c $(GLOBALFLAGS) -c
bkError.o: bkError.c bkError.h $(GLOBALDEPS)
	cc bkError.c $(GLOBALFLAGS) -c
bkGet.o: bkGet.c bkGet.h $(GLOBALDEPS)
	cc bkGet.c $(GLOBALFLAGS) -c
bkSet.o: bkSet.c bkSet.h $(GLOBALDEPS)
	cc bkSet.c $(GLOBALFLAGS) -c

clean: 
	rm -f *.o bk.a
