CFLAGS=-std=c11 -g -fno-common

SRCS=$(wildcard *.c)
OBJS=$(SRCS:.c=.o)

TEST_SRCS=$(wildcard test/*.c)
TESTS=$(TEST_SRCS:.c=.exe)

# Stage 1

chibicc: $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

$(OBJS): chibicc.h

test/macro.exe: chibicc test/macro.c
	./chibicc -c -o test/macro.o test/macro.c
	$(CC) -o $@ test/macro.o -xc test/common

test/%.exe: chibicc test/%.c
	$(CC) -o- -E -P -C test/$*.c | ./chibicc -c -o test/$*.o -
	$(CC) -o $@ test/$*.o -xc test/common

test: $(TESTS)
	for i in $^; do echo $$i; ./$$i || exit 1; echo; done
	test/driver.sh ./chibicc

test-all: test test-stage2

# Stage 2

stage2/chibicc: $(OBJS:%=stage2/%)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

stage2/%.o: chibicc self.py %.c
	mkdir -p stage2/test
	./self.py chibicc.h $*.c > stage2/$*.c
	./chibicc -c -o stage2/$*.o stage2/$*.c

stage2/test/macro.exe: stage2/chibicc test/macro.c
	mkdir -p stage2/test
	./stage2/chibicc -c -o stage2/test/macro.o test/macro.c
	$(CC) -o $@ stage2/test/macro.o -xc test/common

stage2/test/%.exe: stage2/chibicc test/*.c
	mkdir -p stage2/test
	$(CC) -o- -E -P -C test/$*.c | ./stage2/chibicc -c -o stage2/test/$*.o -
	$(CC) -o $@ stage2/test/$*.o -xc test/common

test-stage2: $(TESTS:test/%=stage2/test/%)
	for i in $^; do echo $$i; ./$$i || exit 1; echo; done
	test/driver.sh ./stage2/chibicc

# Misc.

clean:
	rm -f chibicc tmp* $(TESTS) test/*.s test/*.exe
	find * -type f '(' -name '*~' -o -name '*.o' ')' -exec rm {} ';'

.PHONY: test clean test-stage2
