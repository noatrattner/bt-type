CFLAGS = -Wall -Wextra -g

main: main.o
	${CC} ${CFLAGS} -o $@ $<

%.o: %.c
	${CC} ${CFLAGS} -c -o $@ $<

clean: 
	-rm -f main main.o
.PHONY: clean
