CC=gcc

.PHONY: writer

writer: writer.o
	$(CROSS_COMPILE)gcc -o $@ $(LDFLAGS) $^ $(LIBS)

writer.o: writer.c
	$(CROSS_COMPILE)gcc $(INCLUDES) $(CFLAGS) -c $^ -o $@

clean:
	rm -rf *.o writer
