CC = gcc
CFLAGS = -Wall -g
LDFLAGS = -lncurses

SRC = $(wildcard src/*.c)
OBJS = $(SRC:src/%.c=bin/obj/%.o)
TARGET = bin/hexel

all: $(TARGET)

bin/obj/%.o: src/%.c
	$(CC) $(CFLAGS) -c $< $(LDFLAGS) -o $@ 
	
$(TARGET): $(OBJS)
	$(CC) $^ $(LDFLAGS) -o $@

clean:
	$(RM) $(OBJS) $(TARGET)
