// Copyright (c) 2021, João Fé, All rights reserved.
/**
 * @file
 * @brief A compact array that requires \f$ nZ + O(1) \f$ bits of memory, for
 * handling \f$n\f$ elements whose value belongs to some set \f$X\f$, such
 * that \f$ |X| \in [0, 2^Z) \f$. This is useful for saving the \f$n-\f$grams
 * in a compacted way. Consider, for example, that there are \f$1,000,000\f$
 * distinct unigrams and \f$27,000,000\f$ bigrams, and that for each unigram
 * we want to save its probability and an index pointer to the first of the
 * bigrams (saved in another array) that have it as context. Considering that
 * the probability values occupies 32 bits and that the index pointer
 * occupies \f$ \log_2 \lceil 27,000,000 \rceil = 25 \f$ bits, then we need
 * \f$57\f$ bits for each unigram. For saving them in normal C we would need
 * to do something like:
 * @code
 * struct unigram {
 *     float probability;
 *     int first_bigram_index;
 * };
 * struct unigram unigrams[1000000];
 * @endcode
 * However this would occupy \f$ 1,000,000 \times (32 + 32) \f$ bits, with
 * the last 7 bits of the unigram struct being unused. As is should, the
 * array implementation in array.h, requires only \f$ 1,000,000 \times
 * (32 + 25) + O(1) \f$ bits. Using the structure defined above, here is an
 * example:
 * @code
 * struct array *a = array_new(32 + 25, 1000000);
 * struct unigram some_unigram = { 0.5, 1234 };
 * array_set(a, 0, &some_unigram);
 * struct unigram other_unigram = { 0, 0 };
 * array_get(a, 0, &other_unigram);
 * assert(some_unigram.probability = other_unigram.probability);
 * assert(some_unigram.first_bigram_index = other_unigram.first_bigram_index);
 * @endcode
 * @warning
 * Note however that when array_set() is called only the first 57 bits are
 * saved, with the last 7 bits being discarded. As such if we swap the
 * order of the fields declaration in the unigram struct declaration above,
 * this example would not work because the last 7 bits of the probability would
 * be discarded, while all 32 bits of first_bigram_index would be saved!
 *
 * For this situation array_set_compacted() and array_get_extracted() should
 * be used. This functions compact or extract the element as desired before
 * putting or getting the element from the array. For example:
 * @code
 * // field order is swapped
 * struct unigram {
 *     int first_bigram_index;  // it is now the first field
 *     float probability;       // it is now the second field
 * };
 * array_set_compacted(a, 0, { &some_unigram.first_bigram_index,
 *                             &some_unigram.probability },
 *                     { 25, 32 }, 2);
 * array_get_extracted(a, 0, { &other_unigram.first_bigram_index,
 *                             &other_unigram.probability },
 *                     { 25, 32 }, 2);
 * assert(some_unigram.probability = other_unigram.probability);
 * assert(some_unigram.first_bigram_index = other_unigram.first_bigram_index);
 * @endcode
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
 * Create a new array.
 * @param elem_size the number of bits required by each element
 * @param length the array maximum number of elements
 * @return
 */
struct array *array_new(uint8_t elem_size, uint64_t length);

/**
 * Free \p a.
 * @param a
 */
void array_delete(struct array *a);

/**
 * Copy sequentially `a->elem_size` bits starting from the address pointed
 * by \p value into the position/first_child_index of \p a defined by \p at.
 * @param a
 * @param at
 * @param value
 */
void array_set(const struct array *a, uint64_t at, void *value);

/**
 * Copy sequentially `a->elem_size` bits starting from the
 * position/first_child_index of \p a defined by \p at into the address pointed
 * by \p value.
 * @param a
 * @param at
 * @param dest
 */
void array_get(const struct array *a, uint64_t at, void *dest);

/**
 * Set every element of \p a with the value pointed by \p value.
 * @param a 
 * @param value 
 */
void array_fill(struct array *a, void *value);

/**
 * Sort \p a based on \p cmp function. \p cmp function receives two elements
 * of the array and it should return less than 0, 0, or more than 0 if the
 * first argument element is, respectively, less, equal, or greater than the
 * second argument element.
 * @param a
 * @param cmp
 */
void array_sort(struct array *a, int (*cmp)(void *a, void *b));

/**
 * Same as array_sort() but an extra argument can be passed through \p arg.
 * The argument will be passed to the \p cmp function (useful to avoid
 * passing state through global variables).
 * @param a
 * @param cmp
 * @param arg
 */
void array_sort_r(struct array *a, int (*cmp)(void *a, void *b, void *arg),
                  void *arg);

/**
 * Search for \p key value using binary search. The array must be sorted in
 * ascended order in accordance with \p cmp function.
 * @param key value to search for
 * @param a array to be searched
 * @param cmp function that dictates the elements order in the array
 * @param index pass out pointer for returning the first_child_index
 * @return 0 if found or -1 if not found
 */
int8_t
array_bsearch(void *key, struct array *a, int (*cmp)(void *, void *),
              uint64_t *index);

/**
 * Search for \p key value using binary search. The array must be sorted in
 * ascended order in accordance with \p cmp function.
 * @param key value to search for
 * @param a array to be searched
 * @param cmp function that dictates the elements order in the array
 * @param arg optional argument passed to the \p cmp function
 * @param index pass out pointer for returning the first_child_index
 * @return 0 if found or -1 if not found
 */
int8_t array_bsearch_r(void *key, struct array *a,
                       int (*cmp)(void *, void *, void *), void *arg,
                       uint64_t *index);

/**
 * Search for \p key value using binary search. The array must be sorted in
 * ascended order in accordance with \p cmp function.
 * @param key value to search for
 * @param a array to be searched
 * @param cmp function that dictates the elements order in the array
 * @param l left bound (inclusive) of the interval to search within for
 * @param r right bound (exclusive) of the interval to search within for
 * @param index pass out pointer for returning the first_child_index
 * @return 0 if found or -1 if not found
 */
int8_t array_bsearch_within(void *key, struct array *a,
                            int (*cmp)(void *, void *), uint64_t l,
                            uint64_t r,
                            uint64_t *index);

/**
 * Search for \p key value using binary search. The array must be sorted in
 * ascended order in accordance with \p compare function.
 * @param key value to search for
 * @param a array to be searched
 * @param cmp function that dictates the elements order in the array
 * @param arg optional argument passed to the \p compare function
 * @param l left bound (inclusive) of the interval to search within for
 * @param r right bound (exclusive) of the interval to search within for
 * @param index pass out pointer for returning the first_child_index
 * @return 0 if found or -1 if not found
 */
int8_t array_bsearch_r_within(void *key, struct array *a,
                              int (*cmp)(void *, void *, void *),
                              void *arg,
                              uint64_t l, uint64_t r, uint64_t *index);

/**
 * Split the data pointed by \p elem into \p order memory spaces pointed by
 * the \p dests array. The number of bits of data that each memory space
 * receives from \p elem is defined by the \p sizes array.
 * @note This function in mainly used to extract compacted structs or complex
 * objects from \p elem.
 * @param elem address of the compacted element to be extracted into \p order
 * memory spaces
 * @param dests array of destinations where to place the \p order memory
 * spaces on
 * @param sizes array with the sizes of each memory space (in bits)
 * @param n number of memory spaces into which the data is split
 */
void elem_extract(const void *elem, void **dests, const unsigned int *sizes,
                  unsigned int n);

/**
 * Split the data from the \p at-th element of \p a into \p order memory spaces
 * pointed by the \p dests array, as done in \p elem_extract.
 * @see \p elem_extract
 * @param a
 * @param at
 * @param dest
 * @param sizes
 * @param n
 */
void array_get_extracted(const struct array *a, uint64_t at, void **dest,
                         const unsigned int *sizes, unsigned int n);

/**
 * Compact the elements pointed by \p elems (where the size of each element
 * in bits is given by \p sizes array) into the memory space pointed by \p dest
 * . It is the reverse operation of \p elem_extract.
 * @see \p elem_extract
 * @param elems
 * @param dest
 * @param sizes
 * @param n number of elements to be compacted
 */
void elems_compact(void **elems, void *dest, const unsigned int *sizes,
                   unsigned int n);

/**
 * Compact the elements pointed by \p elems as done in \p elems_compact,
 * placing the result at the \p at-th element of \p a.
 * @see \p elems_compact
 * @param a
 * @param at
 * @param elems
 * @param sizes
 * @param n
 */
void array_set_compacted(const struct array *a, uint64_t at, void **elems,
                         const unsigned int *sizes, unsigned int n);

/**
 * Write the array pointed by \p a into file pointed by \p out in binary format.
 * @param a
 * @param out
 */
void array_fwrite(const struct array *a, FILE *out);

/**
 * Write the array in the file pointed by \p out in binary format to the
 * array pointed by \p a.
 * @param a
 * @param in
 * @return
 */
size_t array_fread(struct array *a, FILE *in);

/**
 * Create a new array, containing the slice [\p l,\p r) from \p a.
 * @warning The returned array should be freed by the caller. Use
 * array_delete().
 * @param a
 * @param l
 * @param r
 * @return newly instantiated array
 */
struct array *array_slice(const struct array *a, uint64_t l, uint64_t r);

#endif //NGRAM_LM_ARRAY_H
