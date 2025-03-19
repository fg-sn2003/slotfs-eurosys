CFLAGS=-O3 -fPIC -mclwb -mavx2 -mclflushopt -pthread -mavx512f -Wall -Wno-unused-but-set-variable -Wno-unused-variable -Wno-maybe-uninitialized -Wno-array-bounds -Wno-stringop-overflow
LFLAGS=-ldl -lrt
LIBRARY_NAME = libslot.so
SOURCES = slotfs.c btree.c shm.c arena.c slot.c slot_vfs.c \
		inode.c rbtree.c runtime.c slotfs_func.c lock.c debug.c \
		journal.c mkfs.c recover.c gather.c
OBJECTS = $(addprefix build/, $(notdir $(SOURCES:.c=.o)))
DEPFILES = $(addprefix build/, $(notdir $(SOURCES:.c=.d)))

$(shell mkdir -p build)

.PHONY: all clean test heap

all: $(LIBRARY_NAME) ffile.o  test

test: slotfs.a
	cd tests && $(MAKE)

$(LIBRARY_NAME): slotfs.a lib.c ffile.o
	$(CC) -shared $(CFLAGS) -o build/$@ lib.c build/slotfs.a build/ffile.o $(LFLAGS)

slotfs.a: $(OBJECTS) heap
	ar cr build/slotfs.a $(OBJECTS) build/heap.o build/llist.o

heap: 
	cd heap && $(MAKE)

build/%.o: %.c
	$(CC) -c -o $@ $< $(CFLAGS) $(LFLAGS)
	$(CC) -MM $(CFLAGS) $< > build/$*.d

-include $(DEPFILES)

ffile.o: glibc/ffile.c
	cd glibc && $(MAKE)

clean:
	rm -rf build/*
	cd tests && $(MAKE) clean