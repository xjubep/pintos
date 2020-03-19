CC=gcc
CFLAGS=-g
OBJS=main.o list.o hash.o bitmap.o
TARGET=testlib

$(TARGET): $(OBJS)
	$(CC) -o $(TARGET) $(OBJS)

clean: 
	rm -rf $(OBJS) $(TARGET)

main.o: main.c
list.o: list.h list.c
hash.o: hash.h hash.c
bitmap.o: bitmap.h bitmap.c

