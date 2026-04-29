CC = gcc
CFLAGS = -Wall -pthread -I./include

all: test_queue test_visited indexer query ipc

test_queue: tests/test_queue.c src/queue.c
	$(CC) $(CFLAGS) -o $@ tests/test_queue.c src/queue.c
indexer:
	$(CC) $(CFLAGS) -o indexer src/indexer.c

query:
	$(CC) $(CFLAGS) -o query src/query.c

ipc:
	$(CC) $(CFLAGS) -o ipc src/ipc.c
clean:
	rm -f test_queue test_visited indexer query ipc
	
.PHONY: clean

test_visited: tests/test_visited.c src/visited.c
	$(CC) $(CFLAGS) -o $@ tests/test_visited.c src/visited.c
