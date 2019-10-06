CC=gcc -std=c99
CFLAGS = -D_GNU_SOURCE -ggdb3 -W -Wall -Wextra -Werror -O3
TESTFLAGS = -D_GNU_SOURCE -DLEAF_KEY_AMOUNT=3 -DNODE_KEY_AMOUNT=3 -DTESTING -ggdb3 -W -Wall -Wextra -Werror -O3
LDFLAGS = 
LIBS = 

default: main

%.o: %.c %.h
	$(CC) -c -o $@ $< $(CFLAGS)

test.o: main.c data_types.h btree.h query.h storage_engine.h
	$(CC) -c -o $@ $< $(TESTFLAGS)

main: main.o 
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS) $(LIBS)

test: test.o
	$(CC) $(TESTFLAGS) -o $@ $^ $(LDFLAGS) $(LIBS)

clean:
	rm -f main *.o
	rm -f test *.o
