CC = gcc
CFLAGS = -Wall -Wextra  # Compiler flags
LDFLAGS = -lX11  # Linker flags
TARGET = driver  # Name of the executable

# List of source files
SRCS = receiver.c transmitter.c main.c

# List of object files derived from source files
OBJS = $(SRCS:.c=.o)

# Default target
all: $(TARGET)

# Rule to build the executable
$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $(TARGET) $(LDFLAGS)

# Rule to compile .c files to .o files
%.o: %.c
	$(CC) -c $(CFLAGS) $< -o $@

# Clean rule
clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: all clean
