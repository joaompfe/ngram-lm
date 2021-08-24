// Copyright (c) 2021, João Fé, All rights reserved.

#include <stdlib.h>
#include "ngram.h"

struct ngram *ngram_new(unsigned short n, const struct word **words,
                        const float *probabilities, const float *backoffs)
{
    struct ngram *ngram = malloc(sizeof(struct ngram));
    ngram->word = (words != NULL) ? (struct word *) words[n - 1] : NULL;
    ngram->probability = (probabilities != NULL) ? probabilities[n - 1] : 2.0f;
    ngram->backoff = (backoffs != NULL) ? backoffs[n - 1] : 2.0f;

    if (n > 1)
        ngram->context = ngram_new(--n, words, probabilities, backoffs);
    else
        ngram->context = NULL;

    return ngram;
}

struct ngram *ngram_new_words_only(unsigned short n, const struct word **words)
{
    return ngram_new(n, words, NULL, NULL);
}

struct ngram *ngram_new_empty(unsigned short n)
{
    return ngram_new_words_only(n, NULL);
}

struct ngram *ngram_new_with_context(const struct word *word, float probability,
                                     float backoff,
                                     const struct ngram *context)
{
    struct ngram *ngram = malloc(sizeof(struct ngram));
    ngram->word = (struct word *) word;
    ngram->probability = probability;
    ngram->backoff = backoff;
    ngram->context = (struct ngram *) context;
    return ngram;
}

struct ngram *ngram_new_empty_with_context(const struct ngram *context)
{
    return ngram_new_with_context(NULL, 2.0f, 1.0f, context);
}

struct ngram *
ngram_new_unigram(const struct word *word, float probability, float backoff)
{
    return ngram_new(1, &word, &probability, &backoff);
}

struct ngram *ngram_new_empty_unigram()
{
    return ngram_new_unigram(NULL, 2.0f, 2.0f);
}

void ngram_delete(struct ngram *n)
{
    if (n->context != NULL) {
        ngram_delete(n->context);
        free(n);
    }
}

struct ngram *ngram_set_context(struct ngram *ngram, struct ngram *context)
{
    ngram = ngram->context = context;
    return ngram;
}
