// Copyright (c) 2021, João Fé, All rights reserved.
/**
 * Trie for indexing and querying n-grams. This implementation is based on KenLM, however here the n-grams are not saved
 * in reverse order, making it suitable for next word prediction queries.
 */

#ifndef NGRAM_LM_NGRAM_TRIE_H
#define NGRAM_LM_NGRAM_TRIE_H

#include "array.h"

#include <stdint.h>

typedef uint32_t word_id_type;  // TODD: dynamically adaptable

struct trie {
    unsigned short n;  // (max)-gram. equal to the trie depth
    uint64_t *n_ngrams;
    word_id_type *vocab_lookup;  // the values are 32-bit word hashes and indexes are the word identifiers
    struct array **ngrams;
    uint8_t word_id_size;
};

struct ngram {
    float probability;
    word_id_type word_id;
    uint64_t index;
};

void build_trie_from_arpa(const char *arpa_path, unsigned short order, const char *out_path);
struct trie *new_trie_from_arpa(const char *arpa_path, unsigned short order);
word_id_type get_word_id(const char *word, const struct trie *trie);
struct ngram get_ngram(const struct trie *trie, int n, uint64_t at);
struct ngram query(const struct trie *trie, char const **words, int n);
void trie_fwrite(const struct trie *t, FILE *f);
size_t trie_fread(struct trie *t, FILE *f);

#endif //NGRAM_LM_NGRAM_TRIE_H
