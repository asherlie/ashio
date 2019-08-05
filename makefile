# disable optimizations for mac
OS= $(shell uname)
ifeq ($(OS),Darwin)
OPT=
else
OPT=-O3
endif

CC= gcc
# -I. is added because exmple files are in a different directory
#  than ashio.c
CFLAGS= -lpthread -Wall -Wextra -Wpedantic -I. -g $(OPT)

all: example
ex: ashio.c examples/example.c
ex_str: ashio.c examples/struct_example.c
exp_ex: ashio.c examples/exp_example.c

.PHONY:
example: 
	$(CC) $(CFLAGS) ashio.c examples/example.c -o ex
	$(CC) $(CFLAGS) ashio.c examples/exp_example.c -o exp_ex
	$(CC) $(CFLAGS) ashio.c examples/struct_example.c -o ex_str

.PHONY:
clean:
	rm -f ex ex_str
