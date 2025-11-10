# Compiler and flags
CC=$(CROSS_COMPILE)gcc
CFLAGS=-Wall -Werror

# Target executable
TARGET=writer

# Source file
SRC=writer.c

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC)

clean:
	rm -f $(TARGET) *.o

.PHONY: all clean
