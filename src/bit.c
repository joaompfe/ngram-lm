// Copyright (c) 2021, João Fé, All rights reserved.

#include "bit.h"

// TODO inline
void mov_to(const void *src, int nbits, void *dest, uint8_t dest_offset)
{
    mov(src, 0, dest, dest_offset, nbits);
}

// TODO inline
void mov_from(const void *src, uint8_t src_offset, void *dest, int nbits)
{
    mov(src, src_offset, dest, 0, nbits);
}

void mov(const void *src, uint8_t src_offset, void *dest, uint8_t dest_offset, int nbits)
{
    int i = src_offset / 8;
    while (i--)
        src = (uint8_t *)src + 1;
    i = dest_offset / 8;
    while (i--)
        dest = (uint8_t *)dest + 1;
    src_offset %= 8;
    dest_offset %= 8;

    i = nbits / 8;
    while (i--) {
        SET_BYTE(dest, BYTE_FROM(src, src_offset), dest_offset);
        src = (uint8_t *)src + 1;
        dest = (uint8_t *)dest + 1;
    }
    SET_BYTE(dest, COMBINE(uint8_t, BYTE_FROM(src, src_offset), BYTE_FROM(dest, dest_offset), nbits % 8),
             dest_offset);
}