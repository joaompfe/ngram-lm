// Copyright (c) 2021, João Fé, All rights reserved.

#include "array.h"

#include "bit.h"

#include <stdlib.h>
#include <math.h>
#include <stdio.h>

static void quicksort(struct array *a, uint64_t l, uint64_t r, int64_t (*cmp)(void *, void *, void *), void *arg);
static uint64_t partition(struct array *a, uint64_t l, uint64_t r, int64_t (*cmp)(void *, void *, void *), void *arg);
static inline void swap(struct array *a, uint64_t i, uint64_t j);
static int8_t binary_search(struct array *a, uint64_t l, uint64_t r, void *key,
                            int64_t (*cmp)(void *, void *, void *pVoid), void *arg, uint64_t *index);

static int64_t (*compare_func_no_arg)(void *, void *);

struct array *new_array(uint8_t elem_size, uint64_t length)
{
    struct array *arr = malloc(sizeof(struct array));
    arr->elem_size = elem_size;
    arr->len = length;
    arr->elems = malloc((unsigned long) ceil((double) (elem_size * length) / 8.0) + 8);
    return arr;
}

struct array *delete_array(struct array *arr)
{
    free(arr->elems);
    free(arr);
}

void array_set(const struct array *arr, uint64_t at, void *value)
{
    const uint64_t i = (at * arr->elem_size) / 8;
    const uint8_t offset = (at * arr->elem_size) % 8;
    uint8_t *dest = (uint8_t *) (arr->elems + i);
    mov(value, 0, dest, offset, arr->elem_size);
}

void array_get(const struct array *arr, uint64_t at, void *dest)
{
    const uint64_t i = (at * arr->elem_size) / 8;
    const uint8_t offset = (at * arr->elem_size) % 8;
    uint8_t *src = (uint8_t *) ((uint8_t *)arr->elems + i);
    *((uint8_t *)dest + arr->elem_size / 8) = 0;  // clear last byte for convenience
    mov(src, offset, dest, 0, arr->elem_size);
}

void array_fill(struct array *arr, void *value)
{
    for (int i = 0; i < arr->len; i++)
        array_set(arr, i, value);
}

static inline int64_t compare_func_stub_arg(void *a, void *b, void *arg) {
    return compare_func_no_arg(a, b);
}

void array_sort(struct array *arr, int64_t (*cmp)(void *a, void *b))
{
    compare_func_no_arg = cmp;
    quicksort(arr, 0, arr->len - 1, compare_func_stub_arg, NULL);
}

void array_sort_r(struct array *arr, int64_t (*cmp)(void *a, void *b, void *arg), void *arg)
{
    quicksort(arr, 0, arr->len - 1, cmp, arg);
}

static void quicksort(struct array *a, uint64_t l, uint64_t r, int64_t (*cmp)(void *, void *, void *),
        void *arg)
{
    if (r <= l || (r+1) <= l)
        return;
    uint64_t i = partition(a, l, r, cmp, arg);
    quicksort(a, l, i - 1, cmp, arg);
    quicksort(a, i + 1, r, cmp, arg);
}

static uint64_t partition(struct array *a, uint64_t l, uint64_t r, int64_t (*cmp)(void *, void *, void *), void *arg)
{
    uint64_t i = l - 1;
    uint64_t j = r;
    int length = a->elem_size/8 + (a->elem_size % 8 > 0);
    uint8_t v[length], tmp[length];
    array_get(a, r, v);

    for (;;) {
        while (array_get(a, ++i, tmp), cmp(tmp, v, arg) < 0) ;
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
    const int length = a->elem_size/8 + (a->elem_size % 8 > 0);
    uint8_t tmpi[length], tmpj[length];
    array_get(a, i, tmpi);
    array_get(a, j, tmpj);
    array_set(a, i, tmpj);
    array_set(a, j, tmpi);
}

int8_t array_bsearch(void *key, struct array *arr, int64_t (*compare)(void *, void *), uint64_t *index)
{
    return array_bsearch_within(key, arr, compare, 0, arr->len, index);
}

int8_t array_bsearch_r(void *key, struct array *arr, int64_t (*compare)(void *, void *, void *),
                       void *arg, uint64_t *index)
{
    return array_bsearch_r_within(key, arr, compare, arg, 0, arr->len, index);
}

int8_t array_bsearch_within(void *key, struct array *arr, int64_t (*compare)(void *, void *), uint64_t l, uint64_t r,
                            uint64_t *index)
{
    compare_func_no_arg = compare;
    return binary_search(arr, l, r - 1, key, compare_func_stub_arg, NULL, index);
}

int8_t array_bsearch_r_within(void *key, struct array *arr, int64_t (*compare)(void *, void *, void *), void *arg,
                              uint64_t l, uint64_t r, uint64_t *index)
{
    return binary_search(arr, l, r - 1, key, compare, arg, index);
}

static int8_t binary_search(struct array *a, uint64_t l, uint64_t r, void *key,
                            int64_t (*cmp)(void *, void *, void *), void *arg, uint64_t *index)
{
    uint64_t mid = (l + r) / 2;
    int length = a->elem_size/8 + (a->elem_size % 8 > 0);
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

void elem_extract(const void *elem, void **dests, const int *sizes, int n)
{
    uint8_t *src = (uint8_t *) elem;
    uint8_t offset = 0;
    for (int i = 0; i < n; i++) {
        *((uint8_t *)dests[i] + sizes[i] / 8) = 0;  // clear last byte for convenience
        mov(src, offset, dests[i], 0, sizes[i]);
        src += (offset + sizes[i]) / 8;
        offset = (offset + sizes[i]) % 8;
    }
}

void array_get_extracted(const struct array *arr, uint64_t at, void **dest, const int *sizes, int n)
{
    uint8_t tmp[arr->elem_size / 8 + 1];
    array_get(arr, at, tmp);
    elem_extract(tmp, dest, sizes, n);
}

void elems_compact(void **elems, void *dest, const int *sizes, int n)
{
    uint8_t offset = 0;
    for (int i = 0; i < n; i++) {
        mov_to(elems[i], sizes[i], dest, offset);
        dest += (offset + sizes[i]) / 8;
        offset = (offset + sizes[i]) % 8;
    }
}

void array_set_compacted(const struct array *arr, uint64_t at, void **elems, const int *sizes, int n)
{
    uint8_t tmp[arr->elem_size / 8 + 1];
    elems_compact(elems, tmp, sizes, n);
    array_set(arr, at, tmp);
}

void array_fwrite(const struct array *arr, FILE *out)
{
    fwrite(arr, sizeof(struct array), 1, out);
    size_t n = arr->elem_size * arr->len / 8 + 1;
    fwrite(arr->elems, sizeof(uint8_t), n, out);
}

size_t array_fread(struct array *arr, FILE *in)
{
    size_t read = fread(arr, sizeof(struct array), 1, in);
    if (read != 1) {
        fprintf(stderr, "Error while reading struct array from file.\n");
        return read;
    }
    size_t n = arr->elem_size * arr->len / 8 + 1;
    read = fread(arr->elems, sizeof(uint8_t), n, in);
    if (read != n) {
        fprintf(stderr, "Error while reading array->elems_size.\n");
        return read;
    }
    return 1;
}