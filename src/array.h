// Copyright (c) 2021, João Fé, All rights reserved.
/**
 * A compact array that requires nZ + O(1) bits of memory, when handling n elements of Z bits each. It is mainly useful
 * for situations where Z is different from the size of any C general data type (such as of char, float, double, etc.)
 * and n is too big, possible implying lack of memory.
 */

#ifndef NGRAM_LM_ARRAY_H
#define NGRAM_LM_ARRAY_H

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

struct array {
    uint8_t elem_size;
    uint64_t len;
    uint8_t *elems;
};

/**
 * Creates a new array.
 * @param elem_size the number of bits required by each element
 * @param length the array maximum number of elements
 * @return
 */
struct array *new_array(uint8_t elem_size, uint64_t length);
struct array *delete_array(struct array *arr);

/**
 * Copies sequentially `arr->elem_size` bits starting from the address pointed by `value` into the position/index of
 * `arr` defined by `at`.
 * @param arr
 * @param at
 * @param value
 */
void array_set(const struct array *arr, uint64_t at, void *value);

/**
 * Copies sequentially `arr->elem_size` bits starting from the position/index of `arr` defined by `at` into the address
 * pointed by `value`.
 * @param arr
 * @param at
 * @param dest
 */
void array_get(const struct array *arr, uint64_t at, void *dest);

void array_fill(struct array *arr, void *value);

// TODO change cmp signature to return int instead of int64_t
void array_sort(struct array *arr, int64_t (*cmp)(void *a, void *b));
void array_sort_r(struct array *arr, int64_t (*cmp)(void *a, void *b, void *arg), void *arg);

/**
 * Searches for `key` value using binary search. The array must be sorted in ascended order in accordance with `compare`
 * function.
 * @param key value to search for
 * @param arr array to be searched
 * @param compare function that dictates the elements order in the array
 * @param index pass out pointer for returning the index
 * @return 0 if found or -1 if not found
 */
int8_t array_bsearch(void *key, struct array *arr, int64_t (*compare)(void *, void *), uint64_t *index);

/**
 * Searches for `key` value using binary search. The array must be sorted in ascended order in accordance with `compare`
 * function.
 * @param key value to search for
 * @param arr array to be searched
 * @param compare function that dictates the elements order in the array
 * @param arg optional argument passed to the `compare` function
 * @param index pass out pointer for returning the index
 * @return 0 if found or -1 if not found
 */
int8_t array_bsearch_r(void *key, struct array *arr, int64_t (*compare)(void *, void *, void *), void *arg,
                       uint64_t *index);

/**
 * Searches for `key` value using binary search. The array must be sorted in ascended order in accordance with `compare`
 * function.
 * @param key value to search for
 * @param arr array to be searched
 * @param compare function that dictates the elements order in the array
 * @param l left bound (inclusive) of the interval to search within for
 * @param r right bound (exclusive) of the interval to search within for
 * @param index pass out pointer for returning the index
 * @return 0 if found or -1 if not found
 */
int8_t array_bsearch_within(void *key, struct array *arr, int64_t (*compare)(void *, void *), uint64_t l, uint64_t r,
        uint64_t *index);

/**
 * Searches for `key` value using binary search. The array must be sorted in ascended order in accordance with `compare`
 * function.
 * @param key value to search for
 * @param arr array to be searched
 * @param compare function that dictates the elements order in the array
 * @param arg optional argument passed to the `compare` function
 * @param l left bound (inclusive) of the interval to search within for
 * @param r right bound (exclusive) of the interval to search within for
 * @param index pass out pointer for returning the index
 * @return 0 if found or -1 if not found
 */
int8_t array_bsearch_r_within(void *key, struct array *arr, int64_t (*compare)(void *, void *, void *), void *arg,
        uint64_t l, uint64_t r, uint64_t *index);

/**
 * Splits the data pointed by `elem` into `n` memory spaces pointed by the `dests` array. The number of bits of data
 * that each memory space receives from `elem` is defined by the `sizes` array.
 * @note This function in mainly used to extract compacted structs or complex objects from `elem`.
 * @param elem address of the compacted element to be extracted into `n` memory spaces
 * @param dests array of destinations where to place the `n` memory spaces on
 * @param sizes array with the sizes of each memory space (in bits)
 * @param n number of memory spaces into which the data is split
 */
void elem_extract(const void *elem, void **dests, const unsigned int *sizes, unsigned int n);

/**
 * Splits the data from the `at`-th element of `arr` into `n` memory spaces pointed by the `dests` array, as done in
 * `elem_extract`.
 * @see `elem_extract`
 * @param arr
 * @param at
 * @param dest
 * @param sizes
 * @param n
 */
void array_get_extracted(const struct array *arr, uint64_t at, void **dest, const unsigned int *sizes, unsigned int n);

/**
 * Compacts the elements pointed by `elems` (where the size of each element in bits is given by `sizes` array) into the
 * memory space pointed by `dest`. It is the reverse operation of `elem_extract`
 * @see `elem_extract`
 * @param elems
 * @param dest
 * @param sizes
 * @param n number of elements to be compacted
 */
void elems_compact(void **elems, void *dest, const unsigned int *sizes, unsigned int n);

/**
 * Compacts the elements pointed by `elems` as done in `elems_compact`, placing the result at the `at`-th element of
 * `arr`.
 * @see `elems_compact`
 * @param arr
 * @param at
 * @param elems
 * @param sizes
 * @param n
 */
void array_set_compacted(const struct array *arr, uint64_t at, void **elems, const unsigned int *sizes, unsigned int n);

/**
 * Writes the array pointed by `arr` into file pointed by `out` in binary format.
 * @param arr
 * @param out
 */
void array_fwrite(const struct array *arr, FILE *out);

/**
 * Writes the array in the file pointed by `out` in binary format to the array pointed by `arr`.
 * @param arr
 * @param in
 * @return
 */
size_t array_fread(struct array *arr, FILE *in);

#endif //NGRAM_LM_ARRAY_H