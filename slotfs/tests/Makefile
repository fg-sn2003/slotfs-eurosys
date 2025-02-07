.PHONY: all clean

SRC = $(wildcard *.c)
OBJ = $(SRC:.c=)

all: $(OBJ)

%: %.c
	gcc -o $@ $< ../build/slotfs.a -pthread -lrt -ldl -lpmem -mavx512f -mavx2

clean:
	rm -f $(OBJ)