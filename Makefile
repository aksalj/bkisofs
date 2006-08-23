OBJECTS = bkRead7x.o bkAdd.o bkDelete.o bkExtract.o bkRead.o bkPath.o bkMangle.o bkWrite.o bkWrite7x.o bkTime.o bkSort.o bkError.o bkGet.o

bk.a: $(OBJECTS)
	ar -cr bk.a $(OBJECTS)

bkRead7x.o: bkRead7x.c bkRead7x.h bk.h
	cc bkRead7x.c -Wall -c -g
bkAdd.o: bkAdd.c bkAdd.h bk.h
	cc bkAdd.c -Wall -c -g
bkDelete.o: bkDelete.c bkDelete.h bk.h
	cc bkDelete.c -Wall -c -g
bkExtract.o: bkExtract.c bkExtract.h bk.h
	cc bkExtract.c -Wall -c -g
bkRead.o: bkRead.c bkRead.h bk.h
	cc bkRead.c -Wall -c -g
bkPath.o: bkPath.c bkPath.h bk.h
	cc bkPath.c -Wall -c -g
bkMangle.o: bkMangle.c bkMangle.h bk.h
	cc bkMangle.c -Wall -c -g
bkWrite.o: bkWrite.c bkWrite.h bk.h
	cc bkWrite.c -Wall -c -g
bkWrite7x.o: bkWrite7x.c bkWrite7x.h bk.h
	cc bkWrite7x.c -Wall -c -g
bkTime.o: bkTime.c bkTime.h bk.h
	cc bkTime.c -Wall -c -g
bkSort.o: bkSort.c bkSort.h bk.h
	cc bkSort.c -Wall -c -g
bkError.o: bkError.c bkError.h
	cc bkError.c -Wall -c -g
bkGet.o: bkGet.c
	cc bkGet.c -Wall -c -g

clean: 
	rm -f *.o bk.a
