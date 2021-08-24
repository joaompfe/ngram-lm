// Copyright (c) 2021, João Fé, All rights reserved.

extern "C" {
#include "c/array.h"
}

#include <gtest/gtest.h>

TEST(Array, NewArray)
{
    int elem_size_bits = 4;
    int length = 3;
    int extra_allocated = 8;
    struct array *a = array_new(elem_size_bits, length);

    EXPECT_EQ(a->len, length);
    EXPECT_EQ(a->elem_size, elem_size_bits);
    EXPECT_TRUE(a->elems != nullptr);

    array_delete(a);
}

TEST(Array, SetAndGet)
{
    int elem_size_bits = 1;
    int length = 3;
    struct array *a = array_new(elem_size_bits, length);

    uint8_t elems[] = { 1, 1, 1, 0 };
    uint8_t value;
    array_set(a, 0, &elems[0]);
    array_set(a, 1, &elems[1]);
    array_set(a, 2, &elems[2]);
    array_get(a, 0, &value);
    EXPECT_EQ(value, 1);
    array_get(a, 1, &value);
    EXPECT_EQ(value, 1);
    array_get(a, 2, &value);
    EXPECT_EQ(value, 1);

    value = 0;
    array_set(a, 0, &value);
    array_get(a, 0, &value);
    EXPECT_EQ(value, 0);
    array_get(a, 1, &value);
    EXPECT_EQ(value, 1);
    array_get(a, 2, &value);
    EXPECT_EQ(value, 1);

    value = 0;
    array_set(a, 1, &value);
    array_get(a, 0, &value);
    EXPECT_EQ(value, 0);
    array_get(a, 1, &value);
    EXPECT_EQ(value, 0);
    array_get(a, 2, &value);
    EXPECT_EQ(value, 1);

    value = 1;
    array_set(a, 0, &value);
    array_get(a, 0, &value);
    EXPECT_EQ(value, 1);
    array_get(a, 1, &value);
    EXPECT_EQ(value, 0);
    array_get(a, 2, &value);
    EXPECT_EQ(value, 1);

    value = 0;
    array_set(a, 0, &value);
    array_get(a, 0, &value);
    EXPECT_EQ(value, 0);
    array_get(a, 1, &value);
    EXPECT_EQ(value, 0);
    array_get(a, 2, &value);
    EXPECT_EQ(value, 1);

    value = 0;
    array_set(a, 2, &value);
    array_get(a, 0, &value);
    EXPECT_EQ(value, 0);
    array_get(a, 1, &value);
    EXPECT_EQ(value, 0);
    array_get(a, 2, &value);
    EXPECT_EQ(value, 0);

    array_delete(a);

    elem_size_bits = 3;
    length = 4;
    a = array_new(elem_size_bits, length);
    value = 0;
    array_set(a, 0, &value);
    array_set(a, 1, &value);
    array_set(a, 2, &value);
    array_set(a, 3, &value);
    array_get(a, 0, &value);
    EXPECT_EQ(value, 0);
    array_get(a, 1, &value);
    EXPECT_EQ(value, 0);
    array_get(a, 2, &value);
    EXPECT_EQ(value, 0);
    array_get(a, 3, &value);
    EXPECT_EQ(value, 0);

    value = 3;
    array_set(a, 0, &value);
    value = 7;
    array_set(a, 1, &value);
    value = 1;
    array_set(a, 2, &value);
    array_get(a, 0, &value);
    EXPECT_EQ(value, 3);
    array_get(a, 1, &value);
    EXPECT_EQ(value, 7);
    array_get(a, 2, &value);
    EXPECT_EQ(value, 1);
    array_get(a, 3, &value);
    EXPECT_EQ(value, 0);

    value = 1;
    array_set(a, 3, &value);
    value = 0;
    array_set(a, 0, &value);
    array_get(a, 0, &value);
    EXPECT_EQ(value, 0);
    array_get(a, 1, &value);
    EXPECT_EQ(value, 7);
    array_get(a, 2, &value);
    EXPECT_EQ(value, 1);
    array_get(a, 3, &value);
    EXPECT_EQ(value, 1);

    array_delete(a);

    elem_size_bits = 8;
    a = array_new(elem_size_bits, length);
    value = 1;
    array_set(a, 0, &value);
    array_set(a, 1, &value);
    array_set(a, 2, &value);
    array_set(a, 3, &value);
    array_get(a, 0, &value);
    EXPECT_EQ(value, 1);
    array_get(a, 1, &value);
    EXPECT_EQ(value, 1);
    array_get(a, 2, &value);
    EXPECT_EQ(value, 1);
    array_get(a, 3, &value);
    EXPECT_EQ(value, 1);

    value = 255;
    array_set(a, 0, &value);
    array_set(a, 1, &value);
    array_set(a, 2, &value);
    array_set(a, 3, &value);
    array_get(a, 0, &value);
    EXPECT_EQ(value, 255);
    array_get(a, 1, &value);
    EXPECT_EQ(value, 255);
    array_get(a, 2, &value);
    EXPECT_EQ(value, 255);
    array_get(a, 3, &value);
    EXPECT_EQ(value, 255);

    value = 127;
    array_set(a, 1, &value);
    array_get(a, 0, &value);
    EXPECT_EQ(value, 255);
    array_get(a, 1, &value);
    EXPECT_EQ(value, 127);
    array_get(a, 2, &value);
    EXPECT_EQ(value, 255);
    array_get(a, 3, &value);
    EXPECT_EQ(value, 255);

    value = 0;
    array_set(a, 2, &value);
    array_get(a, 0, &value);
    EXPECT_EQ(value, 255);
    array_get(a, 1, &value);
    EXPECT_EQ(value, 127);
    array_get(a, 2, &value);
    EXPECT_EQ(value, 0);
    array_get(a, 3, &value);
    EXPECT_EQ(value, 255);

    array_delete(a);

    elem_size_bits = 11;
    uint16_t value11;
    a = array_new(elem_size_bits, length);

    value11 = 0;
    array_set(a, 0, &value11);
    value11 = 0;
    array_set(a, 1, &value11);
    value11 = 0;
    array_set(a, 2, &value11);
    value11 = 0;
    array_set(a, 3, &value11);
    array_get(a, 0, &value11);
    EXPECT_EQ(value11, 0);
    array_get(a, 1, &value11);
    EXPECT_EQ(value11, 0);
    array_get(a, 2, &value11);
    EXPECT_EQ(value11, 0);
    array_get(a, 3, &value11);
    EXPECT_EQ(value11, 0);

    value11 = 1024;
    array_set(a, 1, &value11);
    value11 = 1;
    array_set(a, 2, &value11);
    array_get(a, 0, &value11);
    EXPECT_EQ(value11, 0);
    array_get(a, 1, &value11);
    EXPECT_EQ(value11, 1024);
    array_get(a, 2, &value11);
    EXPECT_EQ(value11, 1);
    array_get(a, 3, &value11);
    EXPECT_EQ(value11, 0);

    value11 = 1023;
    array_set(a, 0, &value11);
    array_set(a, 2, &value11);
    value11 = 2;
    array_set(a, 3, &value11);
    array_get(a, 0, &value11);
    EXPECT_EQ(value11, 1023);
    array_get(a, 1, &value11);
    EXPECT_EQ(value11, 1024);
    array_get(a, 2, &value11);
    EXPECT_EQ(value11, 1023);
    array_get(a, 3, &value11);
    EXPECT_EQ(value11, 2);

    array_delete(a);

    elem_size_bits = 24;
    uint32_t value32;
    a = array_new(elem_size_bits, length);

    value32 = 255;
    array_set(a, 0, &value32);
    array_set(a, 1, &value32);
    array_set(a, 2, &value32);
    array_set(a, 3, &value32);
    array_get(a, 0, &value32);
    EXPECT_EQ(value32, 255);
    array_get(a, 1, &value32);
    EXPECT_EQ(value32, 255);
    array_get(a, 2, &value32);
    EXPECT_EQ(value32, 255);
    array_get(a, 3, &value32);
    EXPECT_EQ(value32, 255);

    value32 = 16777215;
    array_set(a, 1, &value32);
    value32 = 0;
    array_set(a, 2, &value32);
    array_get(a, 0, &value32);
    EXPECT_EQ(value32, 255);
    array_get(a, 1, &value32);
    EXPECT_EQ(value32, 16777215);
    array_get(a, 2, &value32);
    EXPECT_EQ(value32, 0);
    array_get(a, 3, &value32);
    EXPECT_EQ(value32, 255);

    array_delete(a);

    elem_size_bits = 31;
    a = array_new(elem_size_bits, length);

    value32 = 1;
    array_set(a, 0, &value32);
    array_set(a, 1, &value32);
    array_set(a, 2, &value32);
    array_set(a, 3, &value32);
    array_get(a, 0, &value32);
    EXPECT_EQ(value32, 1);
    array_get(a, 1, &value32);
    EXPECT_EQ(value32, 1);
    array_get(a, 2, &value32);
    EXPECT_EQ(value32, 1);
    array_get(a, 3, &value32);
    EXPECT_EQ(value32, 1);

    value32 = 2147483647;
    array_set(a, 0, &value32);
    array_set(a, 1, &value32);
    array_set(a, 2, &value32);
    array_set(a, 3, &value32);
    array_get(a, 0, &value32);
    EXPECT_EQ(value32, 2147483647);
    array_get(a, 1, &value32);
    EXPECT_EQ(value32, 2147483647);
    array_get(a, 2, &value32);
    EXPECT_EQ(value32, 2147483647);
    array_get(a, 3, &value32);
    EXPECT_EQ(value32, 2147483647);

    value32 = 1038015966;
    array_set(a, 2, &value32);
    array_get(a, 0, &value32);
    EXPECT_EQ(value32, 2147483647);
    array_get(a, 1, &value32);
    EXPECT_EQ(value32, 2147483647);
    array_get(a, 2, &value32);
    EXPECT_EQ(value32, 1038015966);
    array_get(a, 3, &value32);
    EXPECT_EQ(value32, 2147483647);
}

TEST(Array, GetAndSetLessThan8Bits)
{
    uint8_t elem_size = 4;
    uint64_t length = 10;
    uint8_t elements[] = { 1, 0, 3, 3, 5, 3, 0, 2, 1, 2 };

    struct array *arr = array_new(elem_size, length);
    for (int i = 0; i < length; i++)
        array_set(arr, i, &elements[i]);
    for (int i = 0; i < length; i++) {
        uint8_t value;
        array_get(arr, i, &value);
        EXPECT_EQ(value, elements[i]);
    }

    elements[3] = 1;
    elements[9] = 3;
    elements[0] = 3;
    array_set(arr, 3, (void *) &elements[3]);
    array_set(arr, 9, (void *) &elements[9]);
    array_set(arr, 0, (void *) &elements[0]);
    for (int i = 0; i < length; i++) {
        uint8_t value = 0;
        array_get(arr, i, &value);
        EXPECT_EQ(value, elements[i]);
    }
}

TEST(Array, GetAndSet32Bits)
{
    uint8_t elem_size = 32;
    uint64_t length = 10;
    uint64_t elements[] = { 1, 0, 333333, 3, 5, 3, 0, 2, 1, 2 };

    struct array *arr = array_new(elem_size, length);
    for (int i = 0; i < length; i++)
        array_set(arr, i, (void *) &elements[i]);
    for (int i = 0; i < length; i++) {
        uint32_t value;
        array_get(arr, i, &value);
        EXPECT_EQ(value, elements[i]);
    }
}

TEST(Array, GetAndSet64Bits)
{
    uint8_t elem_size = 64;
    uint64_t length = 10;
    uint64_t elements[] = { 1, 0, 333333, 31232132, 5, 3, 0, 2, 1, 2 };

    struct array *arr = array_new(elem_size, length);
    for (int i = 0; i < length; i++)
        array_set(arr, i, (void *) &elements[i]);
    for (int i = 0; i < length; i++) {
        uint64_t value;
        array_get(arr, i, &value);
        EXPECT_EQ(value, elements[i]);
    }

    elements[3] = 1;
    elements[9] = 3;
    elements[0] = 3;
    array_set(arr, 3, (void *) &elements[3]);
    array_set(arr, 9, (void *) &elements[9]);
    array_set(arr, 0, (void *) &elements[0]);
    for (int i = 0; i < 10; i++) {
        uint64_t value;
        array_get(arr, i, &value);
        EXPECT_EQ(value, elements[i]);
    }
}

struct mock {
    uint32_t a;
    unsigned int b: 3;
    unsigned int c: 3;
};

TEST(Array, GetAndSetMoreThan64Bits)
{
    uint8_t elem_size = 32 + 6; // sizeof(struct mock) * 8;
    uint64_t length = 3;
    struct mock elements[] = {{ 1,       2, 3 },
                              { 123467,  0, 4 },
                              { 7654321, 1, 3 }};

    struct array *arr = array_new(elem_size, length);
    for (int i = 0; i < length; i++)
        array_set(arr, i, (void *) &elements[i]);
    for (int i = 0; i < length; i++) {
        struct mock value;
        array_get(arr, i, &value);
        EXPECT_EQ(value.a, elements[i].a);
        EXPECT_EQ(value.b, elements[i].b);
        EXPECT_EQ(value.c, elements[i].c);
    }

    elements[0].b = 5;
    elements[2].c = 1;
    array_set(arr, 0, (void *) &elements[0]);
    array_set(arr, 2, (void *) &elements[2]);
    for (int i = 0; i < length; i++) {
        struct mock value;
        array_get(arr, i, &value);
        EXPECT_EQ(value.a, elements[i].a);
        EXPECT_EQ(value.b, elements[i].b);
        EXPECT_EQ(value.c, elements[i].c);
    }
}

int uint8_compare(void *a, void *b)
{
    return (*((uint8_t *) a) - *((uint8_t *) b));
}

int uint16_compare(void *a, void *b)
{
    return (*((uint16_t *) a) - *((uint16_t *) b));
}

int uint16_compare_arg(void *a, void *b, void *arg)
{
    uint16_t n = *(uint16_t *) arg;
    if (*((uint16_t *) a) % n == 0 && *((uint16_t *) b) % n != 0)
        return -1;
    if (*((uint16_t *) a) % n != 0 && *((uint16_t *) b) % n == 0)
        return 1;
    return (*((uint16_t *) a) - *((uint16_t *) b));
}

TEST(Array, Sort)
{
    uint8_t elem_size = 3;
    uint64_t length = 10;
    uint8_t elements[] = { 1, 0, 3, 3, 5, 6, 0, 4, 1, 2 };

    struct array *a = array_new(elem_size, length);
    for (int i = 0; i < length; i++)
        array_set(a, i, &elements[i]);
    array_sort(a, uint8_compare);
    for (int i = 1; i < length; i++) {
        uint8_t prev, current;
        array_get(a, i - 1, &prev);
        array_get(a, i, &current);
        EXPECT_TRUE(prev <= current);
    }

    array_delete(a);

    elem_size = 13;
    length = 7;
    uint16_t elems16[] = { 1024, 2048, 0, 0, 1024, 1023, 1 };
    a = array_new(elem_size, length);
    for (int i = 0; i < length; i++)
        array_set(a, i, &elems16[i]);

    array_sort(a, uint16_compare);

    uint16_t value16 = 0;
    array_get(a, 0, &value16);
    EXPECT_EQ(value16, 0);
    array_get(a, 1, &value16);
    EXPECT_EQ(value16, 0);
    array_get(a, 2, &value16);
    EXPECT_EQ(value16, 1);
    array_get(a, 3, &value16);
    EXPECT_EQ(value16, 1023);
    array_get(a, 4, &value16);
    EXPECT_EQ(value16, 1024);
    array_get(a, 5, &value16);
    EXPECT_EQ(value16, 1024);
    array_get(a, 6, &value16);
    EXPECT_EQ(value16, 2048);

    uint16_t base = 2;
    array_sort_r(a, uint16_compare_arg, &base);

    array_get(a, 0, &value16);
    EXPECT_EQ(value16, 0);
    array_get(a, 1, &value16);
    EXPECT_EQ(value16, 0);
    array_get(a, 2, &value16);
    EXPECT_EQ(value16, 1024);
    array_get(a, 3, &value16);
    EXPECT_EQ(value16, 1024);
    array_get(a, 4, &value16);
    EXPECT_EQ(value16, 2048);
    array_get(a, 5, &value16);
    EXPECT_EQ(value16, 1);
    array_get(a, 6, &value16);
    EXPECT_EQ(value16, 1023);
}

TEST(Array, BinarySearch)
{
    uint8_t elem_size = 3;
    uint64_t length = 10;
    uint8_t elements[] = { 1, 0, 3, 3, 5, 6, 0, 4, 1, 2 };

    struct array *arr = array_new(elem_size, length);
    for (int i = 0; i < length; i++)
        array_set(arr, i, &elements[i]);
    array_sort(arr, uint8_compare);

    uint8_t value = 0;
    array_get(arr, 8, &value);
    EXPECT_EQ(value, 5);

    uint64_t index;
    uint8_t key = 5;
    if (array_bsearch(&key, arr, uint8_compare, &index) != 0)
        FAIL();
    EXPECT_EQ(index, 8);
}

TEST(Array, ElemExtract)
{
    uint64_t src = 869032957162;
    uint8_t a, b, c, d;
    unsigned int split[] = { 3, 5, 2, 8 };
    void *dest[] = { &a, &b, &c, &d };
    elem_extract(&src, dest, split, 4);
    EXPECT_EQ(a, 2);
    EXPECT_EQ(b, 29);
    EXPECT_EQ(c, 0);
    EXPECT_EQ(d, 165);
}

TEST(Array, ElemsCompact)
{
    struct tmp {
        uint16_t a;
        uint8_t b;
        uint8_t c;
    };

    struct tmp tmp = { 1020, 123, 132 };
    void *elems[] = { &tmp.a, &tmp.b, &tmp.c };
    uint32_t dest = 0;
    const unsigned int sizes[] = { 10, 7, 9 };
    elems_compact(elems, &dest, sizes, 3);
    EXPECT_EQ(dest, 17428476);
}
