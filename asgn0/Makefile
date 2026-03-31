CC=clang
CFLAGS=-Wall -Wextra -Werror -pedantic
REMOVE         = rm -f
TARGET = hello
  
all: $(TARGET)
  
$(TARGET): $(TARGET).c
	$(CC) $(CFLAGS) -o $(TARGET) $(TARGET).c

clean:
	$(REMOVE) $(TARGET)