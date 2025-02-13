.PHONY: all
all:
	gcc -fPIC -O3 -c heap.c -o ../build/heap.o -mavx2 -mclwb -mclflushopt -mavx512f -pthread -Wall -ldl -lrt 
	gcc -fPIC -O3 -c llist.c -o ../build/llist.o -mavx2 -mclwb -mclflushopt -mavx512f -pthread -Wall -ldl -lrt 
 