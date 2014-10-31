CC=gcc
CFLAGS=-I.
DEPS = # header file 
OBJ = webserver.o
OBJ2 = nonblocking.o

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

webserver: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS)

nonblocking: $(OBJ2)
	$(CC) -o $@ $^ $(CFLAGS)


