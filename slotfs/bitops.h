#ifndef _BITOPS_H
#define _BITOPS_H

#include <stddef.h>
#include <string.h>

static inline void set_bit(size_t nr, void *addr)
{
    char *bmp = (char *)addr;
    bmp[nr / 8] |= 1 << (nr % 8);
}

static inline void clear_bit(size_t nr, void *addr)
{
    char *bmp = (char *)addr;
    bmp[nr / 8] &= ~(1 << (nr % 8));
}

static inline void set_bit_range(size_t nr, void *addr, size_t length) {
    char *bitmap = (char *)addr;
    size_t start_byte = nr / 8;
    size_t start_bit = nr % 8;

    // Handle bits in the first byte
    if (start_bit != 0) {
        size_t remaining_bits_in_byte = 8 - start_bit;
        size_t bits_to_set = (length < remaining_bits_in_byte) ? length : remaining_bits_in_byte;
        bitmap[start_byte] |= ((1 << bits_to_set) - 1) << start_bit;
        start_byte++;
        length -= bits_to_set;
    }

    memset(bitmap + start_byte, 0xff, length / 8);
    length = length % 8;

    // Handle bits in the last byte
    if (length > 0) {
        bitmap[start_byte] |= (1 << length) - 1;
    }
}

static inline void clear_bit_range(size_t nr, void *addr, size_t length)
{
    char *bmp = (char *)addr;
    size_t start_byte = nr / 8;
    size_t start_bit = nr % 8;

    // Handle bits in the first byte
    if (start_bit != 0) {
        size_t remaining_bits_in_byte = 8 - start_bit;
        size_t bits_to_clear = (length < remaining_bits_in_byte) ? length : remaining_bits_in_byte;
        bmp[start_byte] &= ~(((1 << bits_to_clear) - 1) << start_bit);
        start_byte++;
        length -= bits_to_clear;
    }

    // Handle full bytes in the middle
    memset(bmp + start_byte, 0x00, length / 8);
    length = length % 8;

    // Handle bits in the last byte
    if (length > 0) {
        bmp[start_byte] &= ~((1 << length) - 1);
    }
}

static inline int check_bit(size_t nr, void *addr)
{
    char *bmp = (char *)addr;
    return bmp[nr / 8] & (1 << (nr % 8));
}

static inline int next_zero_bit(void *addr, size_t size, size_t offset)
{
    char *bmp = (char *)addr;
    unsigned long i;
    for (i = offset; i < size; i++) {
        if ((bmp[i / 8] & (1 << (i % 8))) == 0) {
            return i;
        }
    }
    return size;
}

static inline int next_set_bit(void *addr, size_t size, size_t offset)
{
    char *bmp = (char *)addr;
    unsigned long i;
    for (i = offset; i < size; i++) {
        if ((bmp[i / 8] & (1 << (i % 8))) == 1) {
            return i;
        }
    }
    return size;
}



#endif // _BITOPS_H