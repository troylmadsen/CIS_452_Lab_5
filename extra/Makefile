filename = messenger
links = -lpthread
objects = messenger.o

messenger: $(objects)
	clang -o $(filename) -Wall $(links) $(objects)

messenger.o: messenger.c
	clang -c -o messenger.o -Wall messenger.c

.PHONY: clean run_r run_w debug
clean:
	-rm $(filename) $(objects)

run: $(filename)
	./$(filename)

debug:
	clang -c -o messenger.o -Wall -g messenger.c
	clang -o ${filename} -Wall $(links) -g $(objects)
