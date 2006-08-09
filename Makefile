OBJECTS = bkRead7x.o bkAdd.o bkDelete.o bkExtract.o bkRead.o bkPath.o bkMangle.o bkWrite.o bkWrite7x.o bkTime.o bkSort.o bkError.o bkGet.o

bk.a: $(OBJECTS)
	ar -cr bk.a $(OBJECTS)

bkRead7x.o: bkRead7x.c bkRead7x.h bk.h
	cc bkRead7x.c -Wall -c
bkAdd.o: bkAdd.c bkAdd.h bk.h
	cc bkAdd.c -Wall -c
bkDelete.o: bkDelete.c bkDelete.h bk.h
	cc bkDelete.c -Wall -c
bkExtract.o: bkExtract.c bkExtract.h bk.h
	cc bkExtract.c -Wall -c
bkRead.o: bkRead.c bkRead.h bk.h
	cc bkRead.c -Wall -c
bkPath.o: bkPath.c bkPath.h bk.h
	cc bkPath.c -Wall -c
bkMangle.o: bkMangle.c bkMangle.h bk.h
	cc bkMangle.c -Wall -c
bkWrite.o: bkWrite.c bkWrite.h bk.h
	cc bkWrite.c -Wall -c
bkWrite7x.o: bkWrite7x.c bkWrite7x.h bk.h
	cc bkWrite7x.c -Wall -c
bkTime.o: bkTime.c bkTime.h bk.h
	cc bkTime.c -Wall -c
bkSort.o: bkSort.c bkSort.h bk.h
	cc bkSort.c -Wall -c
bkError.o: bkError.c bkError.h
	cc bkError.c -Wall -c
bkGet.o: bkGet.c
	cc bkGet.c -Wall -c

clean: 
	rm -f *.o *.a
