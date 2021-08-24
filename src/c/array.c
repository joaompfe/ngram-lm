// Copyright (c) 2021, João Fé, All rights reserved.

#include "array.h"

#include <stdlib.h>
#include <math.h>
#include <stdio.h>

#include "bit.h"
#include "util/log.h"

static void quicksort(struct array *a, uint64_t l, uint64_t r,
                      int (*cmp)(void *, void *, void *), void *arg);

static uint64_t partition(struct array *a, uint64_t l, uint64_t r,
                          int (*cmp)(void *, void *, void *), void *arg);

static inline void swap(struct array *a, uint64_t i, uint64_t j);

static int8_t binary_search(struct array *a, uint64_t l, uint64_t r, void *key,
                            int (*cmp)(void *, void *, void *pVoid),
                            void *arg, uint64_t *index);

static int (*compare_func_no_arg)(void *, void *);

struct array *array_new(uint8_t elem_size, uint64_t length)
{
    struct array *a = malloc(sizeof(struct array));
    a->elem_size = elem_size;
    a->len = length;
    a->elems = malloc(
            (unsigned long) ceil((double) (elem_size * length) / 8.0) + 8);
    return a;
}

void array_delete(struct array *a)
{
    free(a->elems);
    free(a);
}

void array_set(const struct array *a, uint64_t at, void *value)
{
    const uint64_t i = (at * a->elem_size) / 8;
    const uint8_t offset = (at * a->elem_size) % 8;
    uint8_t *dest = (uint8_t *) (a->elems + i);
    mov(value, 0, dest, offset, a->elem_size);
}

void array_get(const struct array *a, uint64_t at, void *dest)
{
    const uint64_t i = (at * a->elem_size) / 8;
    const uint8_t offset = (at * a->elem_size) % 8;
    uint8_t *src = (uint8_t *) ((uint8_t *) a->elems + i);
    *((uint8_t *) dest +
      a->elem_size / 8) = 0;  // clear last byte for convenience
    mov(src, offset, dest, 0, a->elem_size);
}

void array_fill(struct array *a, void *value)
{
    for (unsigned int i = 0; i < a->len; i++)
        array_set(a, i, value);
}

static inline int compare_func_stub_arg(void *a, void *b, void *arg)
{
    return compare_func_no_arg(a, b);
}

void array_sort(struct array *a, int (*cmp)(void *a, void *b))
{
    compare_func_no_arg = cmp;
    quicksort(a, 0, a->len - 1, compare_func_stub_arg, NULL);
}

void array_sort_r(struct array *a, int (*cmp)(void *a, void *b, void *arg),
                  void *arg)
{
    quicksort(a, 0, a->len - 1, cmp, arg);
}

static void quicksort(struct array *a, uint64_t l, uint64_t r,
                      int (*cmp)(void *, void *, void *),
                      void *arg)
{
    if (r <= l || (r + 1) <= l)
        return;
    uint64_t i = partition(a, l, r, cmp, arg);
    quicksort(a, l, i - 1, cmp, arg);
    quicksort(a, i + 1, r, cmp, arg);
}

static uint64_t partition(struct array *a, uint64_t l, uint64_t r,
                          int (*cmp)(void *, void *, void *), void *arg)
{
    uint64_t i = l - 1;
    uint64_t j = r;
    int length = a->elem_size / 8 + (a->elem_size % 8 > 0);
    uint8_t v[length], tmp[length];
    array_get(a, r, v);

    for (;;) {
        while (array_get(a, ++i, tmp), cmp(tmp, v, arg) < 0);
        while (array_get(a, --j, tmp), cmp(v, tmp, arg) < 0)
            if (j == l)
                break;
        if (i >= j)
            break;
        swap(a, i, j);
    }
    swap(a, i, r);
    return i;
}

static inline void swap(struct array *a, uint64_t i, uint64_t j)
{
    const int length = a->elem_size / 8 + (a->elem_size % 8 > 0);
    uint8_t tmpi[length], tmpj[length];
    array_get(a, i, tmpi);
    array_get(a, j, tmpj);
    array_set(a, i, tmpj);
    array_set(a, j, tmpi);
}

int8_t
array_bsearch(void *key, struct array *a, int (*cmp)(void *, void *),
              uint64_t *index)
{
    return array_bsearch_within(key, a, cmp, 0, a->len, index);
}

int8_t array_bsearch_r(void *key, struct array *a,
                       int (*cmp)(void *, void *, void *),
                       void *arg, uint64_t *index)
{
    return array_bsearch_r_within(key, a, cmp, arg, 0, a->len, index);
}

int8_t array_bsearch_within(void *key, struct array *a,
                            int (*cmp)(void *, void *), uint64_t l,
                            uint64_t r,
                            uint64_t *index)
{
    compare_func_no_arg = cmp;
    return binary_search(a, l, r - 1, key, compare_func_stub_arg, NULL, index);
}

int8_t array_bsearch_r_within(void *key, struct array *a,
                              int (*cmp)(void *, void *, void *),
                              void *arg,
                              uint64_t l, uint64_t r, uint64_t *index)
{
    return binary_search(a, l, r - 1, key, cmp, arg, index);
}

static int8_t binary_search(struct array *a, uint64_t l, uint64_t r, void *key,
                            int (*cmp)(void *, void *, void *), void *arg,
                            uint64_t *index)
{
    uint64_t mid = (l + r) / 2;
    int length = a->elem_size / 8 + (a->elem_size % 8 > 0);
    uint8_t tmp[length];
    if (l > r)
        return -1;
    if (array_get(a, mid, tmp), cmp(key, tmp, arg) == 0) {
        *index = mid;
        return 0;
    }
    if (l == r)
        return -1;
    if (array_get(a, mid, tmp), cmp(key, tmp, arg) < 0)
        return binary_search(a, l, mid - 1, key, cmp, arg, index);
    else
        return binary_search(a, mid + 1, r, key, cmp, arg, index);
}

void elem_extract(const void *elem, void **dests, const unsigned int *sizes,
                  unsigned int n)
{
    uint8_t *src = (uint8_t *) elem;
    uint8_t offset = 0;
    for (unsigned int i = 0; i < n; i++) {
        *((uint8_t *) dests[i] +
          sizes[i] / 8) = 0;  // clear last byte for convenience
        mov(src, offset, dests[i], 0, sizes[i]);
        src += (offset + sizes[i]) / 8;
        offset = (offset + sizes[i]) % 8;
    }
}

void array_get_extracted(const struct array *a, uint64_t at, void **dest,
                         const unsigned int *sizes, unsigned int n)
{
    uint8_t tmp[a->elem_size / 8 + 1];
    array_get(a, at, tmp);
    elem_extract(tmp, dest, sizes, n);
}

void elems_compact(void **elems, void *dest, const unsigned int *sizes,
                   unsigned int n)
{
    uint8_t offset = 0;
    for (unsigned int i = 0; i < n; i++) {
        mov_to(elems[i], sizes[i], dest, offset);
        dest = (uint8_t *) dest + (offset + sizes[i]) / 8;
        offset = (offset + sizes[i]) % 8;
    }
}

void array_set_compacted(const struct array *a, uint64_t at, void **elems,
                         const unsigned int *sizes, unsigned int n)
{
    uint8_t tmp[a->elem_size / 8 + 1];
    elems_compact(elems, tmp, sizes, n);
    array_set(a, at, tmp);
}

void array_fwrite(const struct array *a, FILE *out)
{
    fwrite(a, sizeof(struct array), 1, out);
    size_t n = a->elem_size * a->len / 8 + 1;
    fwrite(a->elems, sizeof(uint8_t), n, out);
}

size_t array_fread(struct array *a, FILE *in)
{
    size_t read = fread(a, sizeof(struct array), 1, in);
    if (read != 1) {
        log_error("Exactly 1 struct array should have been read from file, but "
                  "%d was", read);
        return read;
    }
    size_t n = a->elem_size * a->len / 8 + 1;
    a->elems = malloc(
            (unsigned long) ceil((double) (a->elem_size * a->len) / 8.0) + 8);
    read = fread(a->elems, sizeof(uint8_t), n, in);
    if (read != n) {
        log_error("Exactly %d array->elems_sizes should have been read from "
                  "file, but %d were", read);
        return read;
    }
    return 1;
}

struct array *array_slice(const struct array *a, uint64_t l, uint64_t r)
{
    if (r <= l)
        return NULL;
    uint64_t len = r - l;
    struct array *slice = array_new(a->elem_size, len);
    const uint64_t i = (l * a->elem_size) / 8;
    const uint8_t offset = (l * a->elem_size) % 8;
    uint8_t *src = (uint8_t *) ((uint8_t *) a->elems + i);
    unsigned int nbits = a->elem_size * len;
    mov(src, offset, slice->elems, 0, nbits);
    return slice;
}
