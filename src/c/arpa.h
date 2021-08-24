// Copyright (c) 2021, João Fé, All rights reserved.
/**
 * @file
 */

#ifndef NGRAM_LM_ARPA_H
#define NGRAM_LM_ARPA_H

#include <stdio.h>
#include <stdint.h>

struct arpa_section {
    FILE *f;
    unsigned short n;
    fpos_t *begin;
    fpos_t *i;
    uint64_t n_ngrams;
};

struct arpa {
    FILE *f;
    char *path;
    unsigned short order;
    uint64_t *n_ngrams;
    struct arpa_section **sections;
};

struct arpa_ngram {
    float probability;
    unsigned short n;
    char **words;
    float backoff;
};

/**
 * Open ARPA file. Should be closeD when no longer needed with arpa_close().
 * @param path
 * @return
 */
struct arpa *arpa_open(const char *path);

/**
 * Close \p a.
 * @param a
 */
void arpa_close(struct arpa *a);

/**
 * Get \p n -gram section of the ARPA file.
 * @param a
 * @param n
 * @return
 */
struct arpa_section *arpa_get_section(const struct arpa *a, unsigned short n);

uint64_t arpa_for_each_section_line(const struct arpa_section *s,
                                    int (*f)(char *line, void *arg),
                                    void *arg);

uint64_t arpa_for_each_section_linei(const struct arpa_section *s,
                                     int (*f)(char *line, uint64_t i,
                                              void *arg),
                                     void *arg);

uint64_t arpa_for_each_section_ngram(const struct arpa_section *s,
                                     int (*f)(struct arpa_ngram *ngram,
                                              void *arg),
                                     void *arg);

uint64_t arpa_for_each_section_ngrami(const struct arpa_section *s,
                                      int (*f)(struct arpa_ngram *ngram,
                                               uint64_t i, void *arg),
                                      void *arg);

#endif //NGRAM_LM_ARPA_H
