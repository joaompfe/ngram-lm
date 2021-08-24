// Copyright (c) 2021, João Fé, All rights reserved.

#include "arpa.h"

#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <c/util/log.h>

#define WORD_MAX_LENGTH 256
#define KNOWN_PORTUGUESE_WORD_MAX_LENGTH 46

static uint64_t
for_each_line(struct arpa *a, int (*action(char *line, void *arg)), void *arg);

static uint64_t
for_each_linei(struct arpa *a, int (*action)(char *line, uint64_t i, void *arg),
               void *arg);

static int do_nothing(char *line, uint64_t i, void *arg);

static int process_header_line(char *line, uint64_t i, void *arg);

static void arpa_section_delete(struct arpa_section *s);

struct arpa *arpa_open(const char *path)
{
    FILE *f = fopen(path, "r");
    if (f == NULL) {
        log_error("Error while opening %s: %s.\n", path, strerror(errno));
        return NULL;
    }

    struct arpa *a = malloc(sizeof(struct arpa));
    a->f = f;
    a->path = malloc((strlen(path) + 1) * sizeof(char));
    strcpy(a->path, path);
    fpos_t header;
    fgetpos(a->f, &header);
    a->order = for_each_linei(a, do_nothing, a) - 1;
    fsetpos(a->f, &header);
    a->n_ngrams = malloc(a->order * sizeof(uint64_t));
    for_each_linei(a, process_header_line, a);
    a->sections = malloc((a->order + 1) * sizeof(struct arpa_section *));

    struct arpa_section *unigram_section = malloc(sizeof(struct arpa_section));
    unigram_section->n = 1;
    unigram_section->begin = malloc(sizeof(fpos_t));
    unigram_section->i = malloc(sizeof(fpos_t));
    unigram_section->f = fopen(path, "r");
    fgetpos(f, unigram_section->begin);
    *unigram_section->i = *unigram_section->begin;
    unigram_section->n_ngrams = a->n_ngrams[0];
    a->sections[0] = unigram_section;

    for (int i = 1; i < a->order; i++)
        a->sections[i] = NULL;

    return a;
}

void arpa_close(struct arpa *a)
{
    int i = 0;
    fclose(a->f);
    free(a->path);
    free(a->n_ngrams);
    struct arpa_section *s = a->sections[i];
    while (s != NULL) {
        arpa_section_delete(s);
        s = a->sections[++i];
    }
    free(a->sections);
    free(a);
}

static uint64_t
for_each_line(struct arpa *a, int (*action(char *line, void *arg)), void *arg)
{
    char *line = NULL;
    size_t len = 0;
    uint64_t i = 0;
    while (getline(&line, &len, a->f) != -1) {
        if (action(line, arg))
            break;
        i++;
    }
    if (line)
        free(line);
    return i;
}

static uint64_t
for_each_linei(struct arpa *a, int (*action)(char *line, uint64_t i, void *arg),
               void *arg)
{
    char *line = NULL;
    size_t len = 0;
    uint64_t i = 0;
    while (getline(&line, &len, a->f) != -1) {
        if (action(line, i, arg))
            break;
        i++;
    }
    if (line)
        free(line);
    return i;
}

static int do_nothing(char *line, uint64_t i, void *arg)
{
    if (i > 0) {
        if (line[0] == '\n') return 1;
    }
    return 0;
}

static int process_header_line(char *line, uint64_t i, void *arg)
{
    struct arpa *a = (struct arpa *) arg;
    if (i > 0) {
        if (line[0] == '\n') return 1;
        if (sscanf(line, "ngram %*d=%lu", &a->n_ngrams[i - 1]) == EOF)
            return -1;
    }
    return 0;
}

static void
get_section_begin(const struct arpa *a, unsigned short n, fpos_t **begin)
{
    struct arpa_section *s = a->sections[n - 2];
    if (s == NULL)
        get_section_begin(a, n - 1, begin);
    else
        *begin = s->i;
    FILE *f = fopen(a->path, "r");
    fsetpos(f, *begin);

    char sec_title[16];
    snprintf(sec_title, 16, "\\%d-grams:\n", n);
    char *line = NULL;
    size_t len = 0;
    while (getline(&line, &len, f) != -1) {
        if (line[0] == '\\' && strcmp(line, sec_title) == 0)
            return;
        fgetpos(f, *begin);
    }
    *begin = NULL;
}

struct arpa_section *arpa_get_section(const struct arpa *a, unsigned short n)
{
    struct arpa_section *s = a->sections[n - 1];
    if (s != NULL)
        return s;
    s = malloc(sizeof(struct arpa_section));
    s->n = n;
    s->begin = malloc(sizeof(fpos_t));
    s->i = malloc(sizeof(fpos_t));
    s->f = fopen(a->path, "r");
    s->n_ngrams = a->n_ngrams[n - 1];
    get_section_begin(a, n, &s->begin);
    if (s->begin == NULL) {
        log_error("%d-gram section begin not found", n);
        exit(EXIT_FAILURE);
    }
    *s->i = *s->begin;
    fsetpos(s->f, s->begin);
    return s;
}

static void arpa_section_delete(struct arpa_section *s)
{
    fclose(s->f);
    free(s->begin);
    free(s->i);
    free(s);
}

static int
parse_section_line(const char *line, unsigned short n, struct arpa_ngram *ngram)
{
    if (line[0] == '\n')
        return -1;
    ngram->n = n;
    ngram->words = malloc(n * sizeof(char *));
    int nchar_matched;
    if (sscanf(line, "%f%n", &ngram->probability, &nchar_matched) == EOF)
        return -2;
    line += nchar_matched;

    char word[WORD_MAX_LENGTH];
    for (int i = 0; i < n; i++) {
        if (sscanf(line, "%s%n", word, &nchar_matched) == EOF)
            return -3;
        line += nchar_matched;
        ngram->words[i] = malloc((strlen(word) + 1) * sizeof(char));
        strcpy(ngram->words[i], word);
    }

    if (sscanf(line, "%f%n", &ngram->backoff, &nchar_matched) == EOF)
        return -4;

    return 0;
}

static int
(*arpa_for_each_section_ngram_action)(struct arpa_ngram *ngram, void *arg);

static int
arpa_for_each_section_ngram_action_ignoring_i(struct arpa_ngram *ngram,
                                              uint64_t i, void *arg)
{
    return arpa_for_each_section_ngram_action(ngram, arg);
}

uint64_t arpa_for_each_section_ngram(const struct arpa_section *s,
                                     int (*f)(struct arpa_ngram *ngram,
                                              void *arg),
                                     void *arg)
{
    arpa_for_each_section_ngram_action = f;
    return arpa_for_each_section_ngrami(s,
                                        arpa_for_each_section_ngram_action_ignoring_i,
                                        arg);
}

uint64_t arpa_for_each_section_ngrami(const struct arpa_section *s,
                                      int (*f)(struct arpa_ngram *ngram,
                                               uint64_t i, void *arg),
                                      void *arg)
{
    char *line = NULL;
    size_t len = 0;
    uint64_t i = 0;
    fsetpos(s->f, s->begin);
    if (getline(&line, &len, s->f) == -1) {  // read section title
        log_error("Read section title failed because of: %s", strerror(errno));
        return i;
    }
    while (getline(&line, &len, s->f) != -1) {
        struct arpa_ngram ngram;
        int result = parse_section_line(line, s->n, &ngram);
        switch (result) {
            case 0: {
                if (f(&ngram, i, arg))
                    goto end;
                break;
            }
            case -1: {
                goto end;
            }
            default: {
                log_error("%lu-th line '%s' could not be parsed into a "
                          "%d-gram", i, line, s->n);
            }
        }
        i++;
    }

    end:
    if (line)
        free(line);
    return i;
}

static int (*arpa_for_each_section_line_action)(char *line, void *arg);

static int
arpa_for_each_section_line_ignoring_i(char *line, uint64_t i, void *arg)
{
    return arpa_for_each_section_line_action(line, arg);
}

uint64_t arpa_for_each_section_line(const struct arpa_section *s,
                                    int (*f)(char *line, void *arg),
                                    void *arg)
{
    arpa_for_each_section_line_action = f;
    return arpa_for_each_section_linei(s, arpa_for_each_section_line_ignoring_i,
                                       arg);
}

uint64_t arpa_for_each_section_linei(const struct arpa_section *s,
                                     int (*f)(char *line, uint64_t i,
                                              void *arg),
                                     void *arg)
{
    char *line = NULL;
    size_t len = 0;
    uint64_t i = 0;
    fsetpos(s->f, s->begin);
    if (getline(&line, &len, s->f) == -1) {  // read section title
        log_error("Read section title failed because of: %s", strerror(errno));
        return i;
    }
    while (getline(&line, &len, s->f) != -1) {
        if (f(line, i, arg))
            break;
        i++;
    }

    if (line)
        free(line);
    return i;
}
