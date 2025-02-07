#ifndef SHM_H
#define SHM_H

#include <stddef.h>

int shm_map(char *path);
void* global_shm_malloc(size_t size);
void global_shm_free(void* ptr);

#endif // SHM_H