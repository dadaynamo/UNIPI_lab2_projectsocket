CC = gcc
CFLAGS = -Wall -Wextra

SRCS = main.c unboundedqueue.c
OBJS = $(SRCS:.c=.o)
DEPS = unboundedqueue.h util.h

.PHONY: all clean test valgrind

all: main

main: $(OBJS)
	$(CC) $(CFLAGS) $^ -o $@ -lm -pthread -lrt

%.o: %.c $(DEPS)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) main
test1: main
	./main . 1
test2: main
	./main . 5
test3: main
	valgrind --leak-check=full --show-leak-kinds=all --log-file=dump_valgrind.txt --track-origins=yes -s ./main . 5
