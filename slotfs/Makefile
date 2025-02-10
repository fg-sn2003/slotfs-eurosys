CFLAGS=-O3 -fPIC -mclwb -mavx2 -mclflushopt -pthread -Wall -mavx512f -ldl -lrt -lpmem 

LIBRARY_NAME = libslot.so
SOURCES = slotfs.c btree.c shm.c arena.c slot.c slot_vfs.c \
		inode.c rbtree.c runtime.c slotfs_func.c lock.c debug.c \
		journal.c mkfs.c recover.c gather.c
OBJECTS = $(addprefix build/, $(notdir $(SOURCES:.c=.o)))
OBJECTS := $(OBJECTS:.S=.o)

.PHONY: all clean test 

all: $(LIBRARY_NAME) ffile.o  test

test: slotfs.a
	cd tests && $(MAKE)

$(LIBRARY_NAME): slotfs.a lib.c ffile.o
	$(CC) -shared $(CFLAGS) -o build/$@ lib.c build/slotfs.a build/ffile.o

slotfs.a: $(OBJECTS) build/heap.o build/llist.o
	ar cr build/slotfs.a $(OBJECTS) build/heap.o build/llist.o

build/heap.o build/llist.o: 
	cd heap && $(MAKE)

build/%.o: %.c
	$(CC) -c -o $@ $< $(CFLAGS)

build/%.o: %.S
	$(CC) -c -o $@ $< $(CFLAGS)

ffile.o: glibc/ffile.c
	cd glibc && $(MAKE)

clean:
	rm -rf build/*
	cd tests && $(MAKE) clean