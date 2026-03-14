CC      = $(CROSS_COMPILE)gcc
CFLAGS  = -Wall -Wextra -Werror -g
TARGET  = aesdsocket
SRCS    = aesdsocket.c
 
.PHONY: all default clean
 
all default: $(TARGET)
 
$(TARGET): $(SRCS)
	$(CC) $(CFLAGS) -o $@ $^
 
clean:
	rm -f $(TARGET)
