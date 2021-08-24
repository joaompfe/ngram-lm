// Copyright (c) 2021, João Fé, All rights reserved.
/**
 * @file
 */

#ifndef NGRAM_LM_BIT_H
#define NGRAM_LM_BIT_H

#include <stdint.h>

#define LEFT_ONES(type, x) (~(~0U << (x)))
#define RIGHT_ONES(type, x) (~0U << (sizeof(type) * 8 - (x)))
#define LEFT_ZEROS(type, x) (~(LEFT_ONES(type, (x))))
#define RIGHT_ONES_ZEROS(type, x) (~(RIGHT_ONES((type), x)))
#define ONES_FROM(type, from) (RIGHT_ONES(type, sizeof(type) * 8 - (from)))
#define ONES_UNTIL(type, until) (LEFT_ONES(type, ((until) + 1)))
#define ONES_BETWEEN(type, start, end) (ONES_FROM(type, (start)) & ONES_UNTIL \
(type, (end)))
#define COMBINE(type, x0, x1, pos) ((ONES_UNTIL(type, (pos) - 1) & (x0)) | \
(ONES_FROM(type, (pos)) & (x1)))
#define CONCAT(type, x0, x1, pos) ((LEFT_ONES(type, pos) & (x0)) | ((x1) << \
(pos)))
#define LAST(type, x, n) ((RIGHT_ONES(type, (n)) & (x)) >> (sizeof(type) * 8 \
- (n)))
#define FIRST(type, x, n) (LEFT_ONES(type, (n)) & (x))
#define PLACE_BETWEEN(type, place, placement, start, end) ( (ONES_BETWEEN \
(type, start, end) & ((placement) << (start))) |  (~ONES_BETWEEN(type, start, \
end) & (place)) )
#define TAKE_BETWEEN(type, place, start, end) ((~ONES_FROM(type, (end) + 1) & \
(place)) >> (start))
#define BYTE_FROM(x, offset) ( ((*((uint8_t *)(x))) >> (offset)) | ((*( \
(uint8_t *)(x)+1)) << (8 - (offset))) )
#define INC_N_BYTES(x, n) (((uint8_t *)(x)) + (n))
#define SET_BYTE(dest, x, offset) ( {*((uint8_t *)(dest)) = CONCAT(uint8_t, * \
((uint8_t *)(dest)), (x), (offset)); *(((uint8_t *)(dest))+1) = COMBINE       \
(uint8_t, ((x) >> (8-(offset))), *(((uint8_t *)(dest))+1), (offset));} )

/**
 *
 * @warning no bit is cleared. Make sure that dest is initialized as desired
 * (with 0's probably).
 * @param src
 * @param src_offset
 * @param dest
 * @param dest_offset
 * @param nbits
 */
void mov(const void *src, uint8_t src_offset, void *dest, uint8_t dest_offset,
         unsigned int nbits);

/**
 * Copies \p nbits from \p src to \p dest. The copied bits are then shifted
 * \p dest_offset bits over the memory page. The first \p dest_offset bits from
 * \p dest are left as is (not cleared nor changed).
 * @param src
 * @param nbits
 * @param dest
 * @param dest_offset number of bits of offset
 */
void
mov_to(const void *src, unsigned int nbits, void *dest, uint8_t dest_offset);

/**
 * Copies \p nbits starting at \p src + \p src_offset to \p dest. The (\p
 * nbits % 8) bits following the \p nbits bits copied at \p dest are left as
 * is (not cleared nor changed).
 * @param src
 * @param nbits
 * @param dest
 * @param dest_offset number of bits of offset
 */
void
mov_from(const void *src, uint8_t src_offset, void *dest, unsigned int nbits);

#endif //NGRAM_LM_BIT_H
