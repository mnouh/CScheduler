GXX =  gcc #-g -Wall -ansi -pedantic

all: mythreads testSchedule

testSchedule.o: testSchedule.c
	$(GXX) -c testSchedule.c -o testSchedule.o

testSchedule: testSchedule.o
	$(GXX) testSchedule.o -lrt -o testSchedule

mythreads.o: mythreads.c
	$(GXX) -c mythreads.c -o mythreads.o

mythreads: mythreads.o
	$(GXX) mythreads.o -lrt -o mythreads

clean:
	rm -r *.o testSchedule mythreads
