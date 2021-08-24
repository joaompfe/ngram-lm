// Copyright (c) 2021, João Fé, All rights reserved.

extern "C" {
#include "c/bit.h"
}

#include <gtest/gtest.h>

TEST(Bit, MovTo)
{
    uint32_t dst = 36864;
    uint32_t src = 31;
    mov_to(&src, 5, &dst, 7);
    EXPECT_EQ(dst, 40832);

    src = 129;
    dst = 36864;
    mov_to(&src, 8, &dst, 3);
    EXPECT_EQ(dst, 37896);

    src = 796;
    dst = 36864;
    mov_to(&src, 10, &dst, 2);
    EXPECT_EQ(dst, 40048);

    dst = 1879048195;
    src = 17895697;
    mov_to(&src, 25, &dst, 4);
    EXPECT_EQ(dst, 1896943891);
}

TEST(Bit, Mov)
{
    int a, b;
    a = 1;
    b = 0b101;
    mov(&b, 0, &a, 2, 3);
    EXPECT_EQ(a, 21);

    a = 1;
    b = 101;
    mov(&b, 1, &a, 0, 2);
    EXPECT_EQ(a, 2);

    a = 0b101111101001110010100010110;
    b = 0b111010010010001110101011000;
    mov(&a, 3, &b, 5, 13);
    EXPECT_EQ(b, 0b111010010111001010001011000);

    a = 0b010110001001101010001101011010;
    b = 0b101011010101101010010100010011;
    mov(&a, 6, &b, 7, 21);
    EXPECT_EQ(b, 0b101100010011010100011010010011);

    a = 0b010110001001101010001101011010;
    b = 0b101011010101101010010100010011;
    mov(&a, 9, &b, 10, 14);
    EXPECT_EQ(b, 0b101011010011010100010100010011);
}