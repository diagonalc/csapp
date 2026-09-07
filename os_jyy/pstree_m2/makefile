CC = gcc
CFLAGS = -Wall -Wextra -std=c99
TARGET = pstree

all: $(TARGET)

$(TARGET): main.c
	$(CC) $(CFLAGS) -o $@ $^

clean:
	rm -f $(TARGET)

run: $(TARGET)
	./$(TARGET)

test: $(TARGET)
	./$(TARGET)

.PHONY: all clean run test