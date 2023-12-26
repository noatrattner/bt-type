
main: main.o
	${CC} -o $@ $<

%.o: %.c
	${CC} -c -o $@ $<

clean: 
	-rm -f main main.o
.PHONY: clean
