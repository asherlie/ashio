CC= gcc

# -I. is added because exmple files are in a different directory
#  than ashio.c
CFLAGS= -pthread -Wall -Wextra -Werror -Wpedantic -I. -g -O3

OBJ=ashio.o
EX=ex ex_str exp_ex large

all: example

ashio.o: ashio.c
	$(CC) $(CFLAGS) ashio.c -c

ex: $(OBJ) examples/example.c
	$(CC) $(CFLAGS) $(OBJ) examples/example.c -o ex

ex_str: $(OBJ) examples/struct_example.c
	$(CC) $(CFLAGS) $(OBJ) examples/struct_example.c -o ex_str

exp_ex: $(OBJ) examples/exp_example.c
	$(CC) $(CFLAGS) $(OBJ) examples/exp_example.c -o exp_ex

large: $(OBJ) examples/large_input.c
	$(CC) $(CFLAGS) $(OBJ) examples/large_input.c -o large


.PHONY:
example: 
	make $(EX)

.PHONY:
clean:
	rm -f $(OBJ) $(EX)
