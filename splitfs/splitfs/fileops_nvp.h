// Header file for nvmfileops.c

#ifndef __NV_FILEOPS_H_
#define __NV_FILEOPS_H_

#include "nv_common.h"
#include "ledger.h"
#include "nvp_lock.h"
#include "queue_impl.h"
#include "liblfds711/inc/liblfds711.h"

#define _NVP_USE_DEFAULT_FILEOPS NULL

// Declare the real_close function, as it will be called by bg thread
RETT_CLOSE _nvp_REAL_CLOSE(INTF_CLOSE, ino_t serialno, int async_file_closing);
size_t swap_extents(struct NVFile *nvf, int close);
void close_cloexec_files();

/******************* Data Structures ********************/


/******************* Checking ********************/

#define NOSANITYCHECK 1
#if NOSANITYCHECK
	#define SANITYCHECK(x)
#else
	#define SANITYCHECK(x) if(UNLIKELY(!(x))) { ERROR("NVP_SANITY("#x") failed!\n"); exit(101); }
#endif

#define NVP_CHECK_NVF_VALID(nvf) do{					\
		if(UNLIKELY(!nvf->valid)) {				\
			DEBUG("Invalid file descriptor: %i\n", file);	\
			errno = 0;					\
			return -1;					\
		}							\
		else							\
			{						\
				DEBUG("this function is operating on node %p\n", nvf->node); \
				SANITYCHECKNVF(nvf);			\
			}						\
	} while(0)

#define NVP_CHECK_NVF_VALID_WR(nvf) do{					\
		if(UNLIKELY(!nvf->valid)) {				\
			DEBUG("Invalid file descriptor: %i\n", file);	\
			errno = 0;					\
			return -1;					\
		}							\
		else {							\
				DEBUG("this function is operating on node %p\n", nvf->node); \
				SANITYCHECKNVF(nvf);			\
			}						\
	} while(0)

#define SANITYCHECKNVF(nvf)						\
	SANITYCHECK(nvf->valid);					\
	SANITYCHECK(nvf->node != NULL);					\
	SANITYCHECK(nvf->fd >= 0);					\
	SANITYCHECK(nvf->fd < OPEN_MAX);				\
	SANITYCHECK(nvf->offset != NULL);				\
	SANITYCHECK(*nvf->offset >= 0);					\
	SANITYCHECK(nvf->node->length >=0);				\
	SANITYCHECK(nvf->node->maplength >= nvf->node->length);		\
	SANITYCHECK(nvf->node->data != NULL)

/*
  #define SANITYCHECKNVF(nvf)						\
  SANITYCHECK(nvf->valid);						\
  SANITYCHECK(nvf->node != NULL);					\
  SANITYCHECK(nvf->fd >= 0);						\
  SANITYCHECK(nvf->fd < OPEN_MAX);					\
  SANITYCHECK(nvf->offset != NULL);					\
  SANITYCHECK(*nvf->offset >= 0);					\
  SANITYCHECK(nvf->serialno != 0);					\
  SANITYCHECK(nvf->serialno == nvf->node->serialno);			\
  SANITYCHECK(nvf->node->length >=0);					\
  SANITYCHECK(nvf->node->maplength >= nvf->node->length);		\
  SANITYCHECK(nvf->node->data != NULL)
*/

#define EXT4_IOC_DYNAMIC_REMAP	_IOWR('f', 13, struct dynamic_remap_data)

struct dynamic_remap_data {
	int fd1; 
	int fd2;
	loff_t offset1; 
	loff_t offset2;
	const char *start_addr;
	loff_t count;
};

static inline int ioctl_swap_extents(int fd1, int fd2, loff_t offset1, 
									 loff_t offset2, const char *start_addr, loff_t count)
{
	struct dynamic_remap_data data;
	data.fd1 = fd1;
	data.fd2 = fd2;
	data.offset1 = offset1;
	data.offset2 = offset2;
	data.start_addr = start_addr;
	data.count = count;
	MSG("fd1: %d, fd2: %d, offset1: %ld, offset2: %ld, start_addr: %p, count: %ld\n", 
		fd1, fd2, offset1, offset2, start_addr, count);
	MSG("EXT4_IOC_DYNAMIC_REMAP: %llx\n", EXT4_IOC_DYNAMIC_REMAP);
	return ioctl(fd1, EXT4_IOC_DYNAMIC_REMAP, &data);
}


#endif
