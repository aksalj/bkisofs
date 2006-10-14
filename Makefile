OBJECTS = bkRead7x.o bkAdd.o bkDelete.o bkExtract.o bkRead.o bkPath.o bkMangle.o bkWrite.o bkWrite7x.o bkTime.o bkSort.o bkError.o bkGet.o bkSet.o
GLOBALFLAGS = -Wall -g

bk.a: $(OBJECTS)
	ar -cr bk.a $(OBJECTS)

bkRead7x.o: bkRead7x.c bkRead7x.h bk.h
	cc bkRead7x.c $(GLOBALFLAGS) -c
bkAdd.o: bkAdd.c bkAdd.h bk.h
	cc bkAdd.c $(GLOBALFLAGS) -c
bkDelete.o: bkDelete.c bkDelete.h bk.h
	cc bkDelete.c $(GLOBALFLAGS) -c
bkExtract.o: bkExtract.c bkExtract.h bk.h
	cc bkExtract.c $(GLOBALFLAGS) -c
bkRead.o: bkRead.c bkRead.h bk.h
	cc bkRead.c $(GLOBALFLAGS) -c
bkPath.o: bkPath.c bkPath.h bk.h
	cc bkPath.c $(GLOBALFLAGS) -c
bkMangle.o: bkMangle.c bkMangle.h bk.h
	cc bkMangle.c $(GLOBALFLAGS) -c
bkWrite.o: bkWrite.c bkWrite.h bk.h
	cc bkWrite.c $(GLOBALFLAGS) -c
bkWrite7x.o: bkWrite7x.c bkWrite7x.h bk.h
	cc bkWrite7x.c $(GLOBALFLAGS) -c
bkTime.o: bkTime.c bkTime.h bk.h
	cc bkTime.c $(GLOBALFLAGS) -c
bkSort.o: bkSort.c bkSort.h bk.h
	cc bkSort.c $(GLOBALFLAGS) -c
bkError.o: bkError.c bkError.h
	cc bkError.c $(GLOBALFLAGS) -c
bkGet.o: bkGet.c bkGet.h
	cc bkGet.c $(GLOBALFLAGS) -c
bkSet.o: bkSet.c bkSet.h
	cc bkSet.c $(GLOBALFLAGS) -c

clean: 
	rm -f *.o bk.a
