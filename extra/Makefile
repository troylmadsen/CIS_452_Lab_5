r_filename = reader
w_filename = writer
r_objects = reader.o
w_objects = writer.o

reader: $(r_objects)
	clang -o $(r_filename) -Wall $(r_objects)

writer: $(w_objects)
	clang -o ${w_filename} -Wall $(w_objects)

reader.o: reader.c
	clang -c -o reader.o -Wall reader.c

writer.o: writer.c
	clang -c -o writer.o -Wall writer.c

.PHONY: clean run_r run_w debug
clean:
	-rm $(r_filename) $(w_filename) $(r_objects) $(w_objects)

run_r: $(r_filename)
	./$(r_filename)

run_w: $(w_filename)
	./$(w_filename)

debug:
	clang -c -o reader.o -Wall -g reader.c
	clang -c -o writer.o -Wall -g writer.c
	clang -o $(r_filename) -Wall -g $(r_objects)
	clang -o ${w_filename} -Wall -g $(w_objects)
