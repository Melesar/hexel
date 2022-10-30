CC = gcc
CFLAGS = -Wall -g

SRC = $(wildcard src/*.c)
OBJS = $(SRC:src/%.c=bin/obj/%.o)
TARGET = bin/hexel

all: $(TARGET)

bin/obj/%.o: src/%.c
	$(CC) -c $(CFLAGS) $< $(LDFLAGS) -o $@ 
	
$(TARGET): $(OBJS)
	$(CC) $^ $(LDFLAGS) -o $@

clean:
	$(RM) $(OBJS) $(TARGET)
