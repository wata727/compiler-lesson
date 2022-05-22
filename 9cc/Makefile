SRCS=$(wildcard *.c)
OBJS=$(SRCS:.c=.o)

9cc: $(OBJS)
	$(CC) -o 9cc $(OBJS)

$(OBJS): 9cc.h

test: 9cc
	./9cc tests/test.c > tmp.s
	gcc -static -o tmp tmp.s
	./tmp

clean:
	rm -f 9cc *.o *~ tmp*

.PHONY: test clean
