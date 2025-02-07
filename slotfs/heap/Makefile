.PHONY: all
all:
	gcc -fPIC -O2 -c heap.c -o ../build/heap.o -mavx2 -mclwb -mclflushopt -mavx512f -pthread -Wall -ldl -lrt -lpmem
	gcc -fPIC -O2 -c llist.c -o ../build/llist.o -mavx2 -mclwb -mclflushopt -mavx512f -pthread -Wall -ldl -lrt -lpmem
 