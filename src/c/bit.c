// Copyright (c) 2021, João Fé, All rights reserved.

#include "bit.h"

static void set_byte(void *dest, uint8_t x, unsigned short offset)
{
    *((uint8_t *) (dest)) = CONCAT(uint8_t, *((uint8_t *) (dest)), (x),
                                   (offset));
    *(((uint8_t *) (dest)) + 1) = COMBINE(uint8_t, ((x) >> (8 - (offset))),
                                          *(((uint8_t *) (dest)) + 1),
                                          (offset));
}

void
mov_to(const void *src, unsigned int nbits, void *dest, uint8_t dest_offset)
{
    mov(src, 0, dest, dest_offset, nbits);
}

void
mov_from(const void *src, uint8_t src_offset, void *dest, unsigned int nbits)
{
    mov(src, src_offset, dest, 0, nbits);
}

void mov(const void *src, uint8_t src_offset, void *dest, uint8_t dest_offset,
         unsigned int nbits)
{
    unsigned int i = src_offset / 8;
    while (i--)
        src = (uint8_t *) src + 1;
    i = dest_offset / 8;
    while (i--)
        dest = (uint8_t *) dest + 1;
    src_offset %= 8;
    dest_offset %= 8;

    i = nbits / 8;
    while (i--) {
        set_byte(dest, BYTE_FROM(src, src_offset), dest_offset);
        src = (uint8_t *) src + 1;
        dest = (uint8_t *) dest + 1;
    }
    set_byte(dest, COMBINE(uint8_t, BYTE_FROM(src, src_offset),
                           BYTE_FROM(dest, dest_offset), nbits % 8),
             dest_offset);
}
