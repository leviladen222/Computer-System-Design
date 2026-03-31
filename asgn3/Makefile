CC = clang
CFLAGS = -Wall -Wextra -Werror -pedantic

# Object files to build
OBJECTS = queue.o rwlock.o

# Default target
all: $(OBJECTS)

# Rule to compile queue.o
queue.o: queue.c queue.h
	$(CC) $(CFLAGS) -c queue.c -o queue.o

# Rule to compile rwlock.o
rwlock.o: rwlock.c rwlock.h
	$(CC) $(CFLAGS) -c rwlock.c -o rwlock.o

# Rule to clean up object files and executables
clean:
	rm -f $(OBJECTS)

# Phony target to prevent conflicts with files named 'all' or 'clean'
.PHONY: all clean

