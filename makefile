CC= gcc

# -I. is added because exmple files are in a different directory
#  than ashio.c
CFLAGS= -lpthread -Wall -Wextra -Wpedantic -I. -g -O3

OBJ=ashio.o

all: example

ashio.o: ashio.c
	$(CC) $(CFLAGS) ashio.c -c

ex: $(OBJ) examples/example.c
	$(CC) $(CFLAGS) $(OBJ) examples/example.c -o ex

ex_str: $(OBJ) examples/struct_example.c
	$(CC) $(CFLAGS) $(OBJ) examples/struct_example.c -o ex_str

exp_ex: $(OBJ) examples/exp_example.c
	$(CC) $(CFLAGS) $(OBJ) examples/exp_example.c -o exp_ex


.PHONY:
example: 
	make ex
	make ex_str
	make exp_ex

.PHONY:
clean:
	rm -f ex exp_ex ex_str large
