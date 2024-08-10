CC=$(CROSS_COMPILE)gcc

.PHONY: writer

writer: writer.o
	$(CC) -o $@ $(LDFLAGS) $^ $(LIBS)

writer.o: writer.c
	$(CC) $(INCLUDES) $(CFLAGS) -c $^ -o $@

clean:
	rm -rf *.o writer
