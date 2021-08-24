// Copyright (c) 2021, João Fé, All rights reserved.
/**
 * @file
 */

#ifndef NGRAM_LM_NGRAM_H
#define NGRAM_LM_NGRAM_H

#include "word.h"

struct ngram {
    struct ngram *context;
    struct word *word;
    float probability;
    float backoff;
};

/**
 * Create a \p n -gram with \p words and with satellite values defined by \p
 * probabilities and \p backoffs. Must be freed by the caller with
 * ngram_delete().
 * @param n
 * @param words
 * @param probabilities
 * @param backoffs
 * @return
 */
struct ngram *ngram_new(unsigned short n, const struct word **words,
                        const float *probabilities, const float *backoffs);

/**
 * Create a \p n -gram only with \p words. Satellite values are left
 * undefined. Must be freed by the caller with ngram_delete().
 * @param n
 * @param words
 * @return
 */
struct ngram *ngram_new_words_only(unsigned short n, const struct word **words);

/**
 * Allocate memory for a \p n -gram, but do not initialize any word or satellite
 * value. Must be freed by the caller with ngram_delete().
 * @param n
 * @return
 */
struct ngram *ngram_new_empty(unsigned short n);

/**
 * Create a gram with \p word, \p probability and \p backoff values, and
 * prepend it a n-gram defined by \p context. Useful to build n-grams
 * incrementally. Must be freed by the caller with ngram_delete().
 * @warning If the returned n-gram is freed by the caller, the assign \p
 * context is also freed. To avoid that set the context field of the returned
 * n-gram to NULL before calling ngram_delete().
 * @param word
 * @param probability
 * @param backoff
 * @param context
 * @return
 */
struct ngram *ngram_new_with_context(const struct word *word, float probability,
                                     float backoff,
                                     const struct ngram *context);

/**
 * Allocate memory for a gram without initializing any word or satellite
 * value, but assigning a n-gram context defined by \p context.
 * @param context
 * @return
 */
struct ngram *ngram_new_empty_with_context(const struct ngram *context);

/**
 * Create a unigram. Must be freed by the caller with ngram_delete().
 * @param word
 * @param probability
 * @param backoff
 * @return
 */
struct ngram *
ngram_new_unigram(const struct word *word, float probability, float backoff);

/**
 * Allocate memory for a unigram without initializing any field. Must be
 * freed by the caller with ngram_delete().
 * @return
 */
struct ngram *ngram_new_empty_unigram();

/**
 * Free \p ngram.
 * @param n
 */
void ngram_delete(struct ngram *ngram);

/**
 * Set the context of \p ngram with \p context and return \p ngram.
 * @param ngram
 * @param context
 * @return the same ngram passed as argument (with new context).
 */
struct ngram *ngram_set_context(struct ngram *ngram, struct ngram *context);

#endif //NGRAM_LM_NGRAM_H
