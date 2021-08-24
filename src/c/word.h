// Copyright (c) 2021, João Fé, All rights reserved.
/**
 * @file
 */

#ifndef NGRAM_LM_WORD_H
#define NGRAM_LM_WORD_H

#include <stdint.h>

typedef uint32_t word_id_type;
typedef uint64_t word_hash_type;

struct word {
    word_hash_type hash;
    char *text;
};

inline const char *word_get_text(const struct word *word)
{
    return word->text;
}

#endif //NGRAM_LM_WORD_H
