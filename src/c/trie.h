// Copyright (c) 2021, João Fé, All rights reserved.
/**
 * @file
 * @brief Trie for indexing and querying n-grams. This implementation is
 * based on KenLM, however here the n-grams are not saved in reverse order,
 * making it suitable for next word prediction queries.
 */

#ifndef NGRAM_LM_TRIE_H
#define NGRAM_LM_TRIE_H

#include <stdint.h>

#include "arpa.h"
#include "array.h"
#include "ngram.h"
#include "word.h"

struct trie {
    unsigned short order;
    uint64_t *n_ngrams;
    struct word *vocab_lookup;
    struct array **arrays;      /// sorted ngram arrays
};

struct array_record {
    float probability;
    word_id_type word_id;
    uint64_t first_child_index;
};

/**
 * Create a new trie from the ARPA file specified by \p arpa, with the maximum
 * order of \p order.
 * @warning The trie must be freed by the caller.
 * @param order
 * @param arpa
 * @return
 */
struct trie *trie_new_from_arpa(unsigned short order, const struct arpa *arpa);

/**
 * Create a new trie from the ARPA file specified by \p arpa_path, with the
 * maximum order of \p order.
 * @warning The trie must be freed by the caller.
 * @param order
 * @param arpa
 * @return
 */
struct trie *
trie_new_from_arpa_path(unsigned short order, const char *arpa_path);

/**
 * Free trie \p t.
 * @param t
 */
void trie_delete(struct trie *t);

/**
 * Try to find the *\p n-gram defined by \p words. Since the complete n-gram
 * might not be found in the trie, \p n is set with the length of the returned
 * n-gram.
 * @warning The returned ngram must be freed by the caller. Use
 * ngram_delete().
 * @param t
 * @param words array of \p n words/grams.
 * @param n length of the n-gram to be queried, as parameter. length of the
 * returned n-gram, as return-value.
 * @return n-gram found.
 */
struct ngram *
trie_query_ngram(const struct trie *t, char const **words, int *n);

/**
 * Write trie model to file pointed by \p f.
 * @param t
 * @param f
 */
void trie_fwrite(const struct trie *t, FILE *f);

/**
 * Read trie model from file pointed by \p f.
 * @warning *\p t should be freed by the caller. Use trie_delete().
 * @param t
 * @param f
 * @return
 */
size_t trie_fread(struct trie **t, FILE *f);

/**
 * Save trie model to disk on the file located by \p path.
 * @param t
 * @param path
 * @return 0 if no error occurred.
 */
int trie_save(const struct trie *t, const char *path);

/**
 * Load trie model from disk.
 * @warning *\p t should be freed by the caller. Use trie_delete().
 * @param path
 * @param t
 * @return 0 if no error occurred. Other value if an error occurred.
 */
int trie_load(const char *path, struct trie **t);

word_id_type
trie_get_word_id_from_text(const struct trie *t, const char *word_text);

/**
 * Obtain the text correspondent to word \p id. The rational of arguments
 * \p dest and \p n is the same as in \p <a href="https://en.cppreference
 * .com/w/c/string/byte/strncpy">strncpy</a>.
 * @warning To avoid incomplete copies, make sure \p dest is sufficiently
 * large buffer.
 * @param t
 * @param id
 * @param dest destination where to copy the word text.
 * @param n number of characters to be copied to \p dest.
 * @return a pointer to string \p dest.
 */
char *
trie_word_textncpy(const struct trie *t, word_id_type id, char *dest, size_t n);

/**
 * Get next word prediction given the \p n-length context given by \p words.
 * @param t
 * @param words prediction context, of length \p n.
 * @param n the length of context \p words.
 * @return next word prediction.
 */
struct word *trie_get_nwp(const struct trie *t, const char **words, int n);

/**
 * Get top \p k next predictions predictions given the \p n-length context given by \p
 * words.
 * @param t
 * @param words prediction context, of length \p n.
 * @param n the length of context \p words.
 * @return next predictions prediction.
 */
void trie_get_k_nwp(const struct trie *t, const char **words, int n,
                    unsigned short k, struct word **predictions);

/**
 * Give the smoothed probability of the \p n -gram given by \p words.
 * @param words
 * @param n
 * @return
 */
float trie_ngram_probability(const struct trie *t, const char **words, int n);

#endif //NGRAM_LM_TRIE_H
