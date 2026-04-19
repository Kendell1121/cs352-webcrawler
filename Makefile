CC = gcc
CFLAGS = -Wall -pthread -I./include

test_queue: tests/test_queue.c src/queue.c
	$(CC) $(CFLAGS) -o $@ tests/test_queue.c src/queue.c

clean:
	rm -f test_queue

.PHONY: clean

test_visited: tests/test_visited.c src/visited.c
	$(CC) $(CFLAGS) -o $@ tests/test_visited.c src/visited.c
