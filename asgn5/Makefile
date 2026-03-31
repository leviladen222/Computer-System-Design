EXECBIN  = httpproxy
SOURCES  = httpproxy.c cache.c
OBJECTS  = $(SOURCES:%.c=%.o)
FORMATS  = $(SOURCES:%.c=%.fmt)
LIBS     = asgn5_helper_funcs.a

CC       = clang
FORMAT   = clang-format
CFLAGS   = -Wall -Werror -Wextra -pedantic -Wstrict-prototypes

.PHONY: all clean format

all: $(EXECBIN)

$(EXECBIN): $(OBJECTS) $(LIBS)
	$(CC) -o $@ $^

%.o : %.c
	$(CC) $(CFLAGS) -c $<

clean:
	rm -f $(EXECBIN) $(OBJECTS) $(FORMATS)

format: $(FORMATS)

%.fmt: %.c %.h
	$(FORMAT) -i $<
	touch $@