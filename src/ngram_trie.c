// Copyright (c) 2021, João Fé, All rights reserved.

#include "ngram_trie.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <qlibc.h>

#define ceil_log2(x) ((uint8_t) ceil(log2(x)))
#define WORD_MAX_LENGTH 256
#define KNOWN_PORTUGUESE_WORD_MAX_LENGTH 46

/**
 * Used during trie creation.
 */
struct tmp_ngram {
    float probability;
    word_id_type word_id;
    uint64_t context_id;
};

static void read_n_ngrams(int order, FILE* header, uint64_t *n_ngrams);
static void create_vocab_lookup(uint64_t n_unigrams, FILE *body, struct trie *trie);
static int cmp_vocab_words(const void *a, const void *b);
static void populate_ngrams(int order, FILE *body, struct trie *trie);
static void populate_unigrams(int order, FILE *body, struct trie *trie);
static int64_t cmp_tmp_grams(void *a, void *b, void *arg);
static void parse_ngram_definition(const char *line, int n, const struct trie * trie, float *prob,
                                   uint64_t *context_id, word_id_type *word_id);
static void set_ngram(const struct trie *trie, int n, uint64_t at, struct ngram *ngram);
static int64_t cmp_ngram(void *a, void *b, void *arg);
static uint64_t get_context_id(const word_id_type *context_words_ids, int context_len, const struct trie *trie);
static struct tmp_ngram get_tmp_ngram(const struct trie *trie, int n, uint64_t at);
static void set_tmp_ngram(const struct trie *trie, int n, uint64_t at, struct tmp_ngram *ngram);

static void fill_in_indexes(const struct trie *trie, int n);

static void reduce_last_order_ngram_array(struct trie *trie);

struct trie *new_trie_from_arpa(const char *file_path, const unsigned short order) {
    FILE *fp = fopen(file_path, "r");
    if (fp == NULL)
        exit(EXIT_FAILURE);

    struct trie *t = malloc(sizeof(struct trie));
    t->n = order;
    t->n_ngrams = malloc(order * sizeof(int));
    t->ngrams = malloc(order * sizeof(struct array *));

    read_n_ngrams(order, fp, t->n_ngrams);
    t->word_id_size = ceil_log2(t->n_ngrams[0]);

    fpos_t unigram_section;
    fgetpos(fp, &unigram_section);
    create_vocab_lookup(t->n_ngrams[0], fp, t);
    fsetpos(fp, &unigram_section);  // set position indicator of stream to the beginning of 1-grams section

    populate_ngrams(order, fp, t);

    fclose(fp);

    return t;
}

#define FOR_SECTION_LINE(sec, title, do) \
char *line = NULL;                       \
size_t len = 0;                          \
if (getline(&line, &len, sec) == -1) exit(EXIT_FAILURE); \
if (strcmp(line, title) != 0) {          \
    fprintf(stderr, "'%s' expected at the beginning of the ARPA file, but '%s' was found instead.", title, line); \
    exit(EXIT_FAILURE);                  \
}                                        \
uint64_t i = 0;                          \
while (getline(&line, &len, sec) != -1 && line[0] != '\n') {                                                      \
    do                                   \
    i++;                                 \
}                                        \
if (line)                                \
    free(line);

#define PROGRESS_BAR(desc, i, total) \
if (((i) + 1) % ((total) / 100) == 0) { \
printf("\r%s: %d%%", desc, (int) (((i) + 1) * 100 / (total))); \
fflush(stdout);                      \
}

static void read_n_ngrams(const int order, FILE *header, uint64_t *n_ngrams)
{
    FOR_SECTION_LINE(header, "\\data\\\n",
        if (i < order && sscanf(line, "ngram %*d=%lu", &n_ngrams[i]) == EOF)
            exit(EXIT_FAILURE);
    )
}

static void create_vocab_lookup(const uint64_t n_unigrams, FILE *body, struct trie *trie)
{
    trie->vocab_lookup = malloc(n_unigrams * sizeof(word_id_type));
    char word[WORD_MAX_LENGTH];
    FOR_SECTION_LINE(body, "\\1-grams:\n",
        double prob;
        if (sscanf(line, "%lf%s%*f", &prob, word) == EOF)
            exit(EXIT_FAILURE);

        if (strlen(word) > KNOWN_PORTUGUESE_WORD_MAX_LENGTH) {
            fprintf(stderr, "a word with %lu characters was found at the %lu-th line of the 1-grams section",
                    strlen(word), i+1);
        }

        word_id_type hash = qhashmurmur3_32(word, strlen(word));
        trie->vocab_lookup[i] = hash;
    )
    qsort(trie->vocab_lookup, n_unigrams, sizeof(word_id_type), cmp_vocab_words);
}

static int cmp_vocab_words(const void *a, const void *b)
{
    word_id_type id_a = *(word_id_type *)a, id_b = *(word_id_type *)b;
    if (id_a < id_b) return -1;
    else if (id_a > id_b) return 1;
    else return 0;
}

static void populate_ngrams(const int order, FILE *body, struct trie *trie)
{
    populate_unigrams(order, body, trie);
    for (int n = 2; n <= order; n++) {
        printf("\nPopulating %d-grams", n);

        uint8_t index_size = ceil_log2((n < order) ? trie->n_ngrams[n] : trie->n_ngrams[n-2]);
        trie->ngrams[n-1] = new_array(sizeof(float) * 8 + trie->word_id_size + index_size, trie->n_ngrams[n-1] + 1);
        printf("\nArray allocated\n");

        char section_tile[16];
        snprintf(section_tile, 16, "\\%d-grams:\n", n);
        FOR_SECTION_LINE(body, section_tile,
            struct tmp_ngram tmp;
            tmp.context_id = 0;
            tmp.probability = 0;
            tmp.word_id = 0;
            parse_ngram_definition(line, n, trie, &tmp.probability, &tmp.context_id, &tmp.word_id);
            set_tmp_ngram(trie, n, i, &tmp);
            PROGRESS_BAR("Reading ARPA", i, trie->n_ngrams[n-1])
        )
        struct tmp_ngram dummy = {0, trie->n_ngrams[0], trie->n_ngrams[n - 2] }; // TODO shouldn't these be subtracted with 1, won't they overflow on limit situation?
        set_tmp_ngram(trie, n, i, &dummy);

        printf("\nSorting... This might take a while...\n");
        const int tmp_sizes[] = { sizeof(float) * 8, trie->word_id_size, ceil_log2(trie->n_ngrams[n-2]) };
        array_sort_r(trie->ngrams[n - 1], cmp_tmp_grams, (void *) tmp_sizes);

        fill_in_indexes(trie, n - 1);
    }
    printf("\n");
    reduce_last_order_ngram_array(trie);
}

static void populate_unigrams(const int order, FILE *body, struct trie *trie)
{
    printf("\nPopulating 1-grams");
    trie->ngrams[0] = new_array(ceil_log2(trie->n_ngrams[1]) + sizeof(float) * 8, trie->n_ngrams[0] + 1);
    printf("\nArray allocated\n");
    struct unigram {
        float probability;
        uint64_t index;
    };
    char word[WORD_MAX_LENGTH];
    FOR_SECTION_LINE(body, "\\1-grams:\n",
        float prob;
        if (sscanf(line, "%f%s%*f", &prob, word) == EOF)
         exit(EXIT_FAILURE);

        word_id_type id = get_word_id(word, trie);
        struct unigram unigram;
        unigram.probability = prob;
        unigram.index = 0;
        array_set(trie->ngrams[0], id, &unigram);
        PROGRESS_BAR("Reading ARPA", i, trie->n_ngrams[0])
    )
}

static void reduce_last_order_ngram_array(struct trie *trie)
{
    const int n = trie->n;
    struct array *old = trie->ngrams[n-1];
    struct array *new = new_array(sizeof(float) * 8 + trie->word_id_size, trie->n_ngrams[n-1]);
    for (int i = 0; i < trie->n_ngrams[n-1]; i++) {
        trie->ngrams[n-1] = old;
        struct tmp_ngram ngram = get_tmp_ngram(trie, n, i);
        trie->ngrams[n-1] = new;
        set_ngram(trie, n, i, (struct ngram *) &ngram);
        PROGRESS_BAR("Reducing N-gram array", i, trie->n_ngrams[n-1])
    }
    delete_array(old);
}

static void fill_in_indexes(const struct trie *trie, int n)
{
    n++;
    struct ngram dummy = get_ngram(trie, n-1, trie->n_ngrams[n-2]);
    dummy.index = trie->n_ngrams[n-1];
    set_ngram(trie, n-1, trie->n_ngrams[n-2], &dummy);

    uint64_t context_id = get_tmp_ngram(trie, n, 0).context_id;
    struct ngram parent_ngram = get_ngram(trie, n-1, context_id);
    parent_ngram.index = 0;
    set_ngram(trie, n-1, context_id, &parent_ngram);
    for (int i = 0; i < trie->n_ngrams[n-1]; i++) {
        while (context_id < get_tmp_ngram(trie, n, i).context_id) {
            context_id++;
            parent_ngram = get_ngram(trie, n-1, context_id);
            parent_ngram.index = i;
            set_ngram(trie, n-1, context_id, &parent_ngram);
        }
        PROGRESS_BAR("Filling in the indexes", i, trie->n_ngrams[n-1])
    }
}

static int64_t cmp_tmp_grams(void *a, void *b, void *arg)
{
    struct tmp_ngram tmp;
    const int *sizes = (const int*) arg;
    void *dest[] = { &tmp.probability, &tmp.word_id, &tmp.context_id };
    // TODO create extract_ngram function for these situations
    elem_extract(a, dest, sizes, 3);
    uint64_t a_context_id = tmp.context_id;
    word_id_type a_id = tmp.word_id;
    elem_extract(b, dest, sizes, 3);
    uint64_t b_context_id = tmp.context_id;
    word_id_type b_id = tmp.word_id;
    if (a_context_id < b_context_id) return -1;
    else if (a_context_id > b_context_id) return 1;
    else {
        if (a_id < b_id) return -1;
        else if (a_id > b_id) return 1;
        else return 0;
    };
}

static void parse_ngram_definition(const char *line, const int n, const struct trie * trie, float *prob,
        uint64_t *context_id, word_id_type *word_id)
{
    int nchar_matched;
    if (sscanf(line, "%f%n", prob, &nchar_matched) == EOF)
        exit(EXIT_FAILURE);
    line += nchar_matched;

    char word[WORD_MAX_LENGTH];
    word_id_type ids[n];
    for (int i = 0; i < n; i++) {
        if (sscanf(line, "%s%n", word, &nchar_matched) == EOF)
            exit(EXIT_FAILURE);
        line += nchar_matched;

        ids[i] = get_word_id(word, trie);
    }
    *context_id = get_context_id(ids, n - 1, trie);
    *word_id = ids[n - 1];
}

word_id_type get_word_id(const char *word, const struct trie *trie)
{
    word_id_type hash = qhashmurmur3_32(word, strlen(word));
    void *idx = bsearch(&hash, trie->vocab_lookup, trie->n_ngrams[0],
                        sizeof(word_id_type), cmp_vocab_words);
    return (idx - (void *) trie->vocab_lookup) / sizeof(word_id_type);
}

struct ngram get_ngram(const struct trie *trie, const int n, uint64_t at)
{
    struct ngram ngram = { 0, 0, 0 };
    if (n == 1) {
        void *dest[] = { &ngram.probability, &ngram.index };
        const int sizes[] = { sizeof(float) * 8, ceil_log2(trie->n_ngrams[n]) };
        array_get_extracted(trie->ngrams[n-1], at, dest, sizes, 2);
    } else if (n < trie->n) {
        void *dest[] = { &ngram.probability, &ngram.word_id, &ngram.index };
        const int sizes[] = { sizeof(float) * 8, trie->word_id_size, ceil_log2(trie->n_ngrams[n]) };
        array_get_extracted(trie->ngrams[n-1], at, dest, sizes, 3);
    } else {
        void *dest[] = { &ngram.probability, &ngram.word_id };
        const int sizes[] = { sizeof(float) * 8, trie->word_id_size };
        array_get_extracted(trie->ngrams[n-1], at, dest, sizes, 2);
    }
    return ngram;
}

static void set_ngram(const struct trie *trie, const int n, uint64_t at, struct ngram *ngram)
{
    uint8_t tmp[trie->ngrams[n-1]->elem_size / 8 + 1];
    if (n == 1) {
        void *elems[] = { &ngram->probability, &ngram->index };
        const int sizes[] = { sizeof(float) * 8, ceil_log2(trie->n_ngrams[n]) };
        elems_compact(elems, tmp, sizes, 2);
    } else if (n < trie->n) {
        void *elems[] = { &ngram->probability, &ngram->word_id, &ngram->index };
        const int sizes[] = { sizeof(float) * 8, trie->word_id_size, ceil_log2(trie->n_ngrams[n]) };
        elems_compact(elems, tmp, sizes, 3);
    } else {
        void *elems[] = { &ngram->probability, &ngram->word_id };
        const int sizes[] = { sizeof(float) * 8, trie->word_id_size };
        elems_compact(elems, tmp, sizes, 2);
    }
    array_set(trie->ngrams[n-1], at, tmp);
}

static struct tmp_ngram get_tmp_ngram(const struct trie *trie, const int n, uint64_t at)
{
    struct tmp_ngram ngram = {0, 0, 0 };
    if (n == 1) {
        void *dest[] = { &ngram.probability, &ngram.context_id };
        const int sizes[] = { sizeof(float) * 8, ceil_log2(trie->n_ngrams[n]) };
        array_get_extracted(trie->ngrams[n-1], at, dest, sizes, 2);
    } else {
        void *dest[] = { &ngram.probability, &ngram.word_id, &ngram.context_id };
        const int sizes[] = { sizeof(float) * 8, trie->word_id_size,
                              ceil_log2((n < trie->n) ? trie->n_ngrams[n] : trie->n_ngrams[n-2]) };
        array_get_extracted(trie->ngrams[n-1], at, dest, sizes, 3);
    }
    return ngram;
}

static void set_tmp_ngram(const struct trie *trie, const int n, uint64_t at, struct tmp_ngram *ngram)
{
    uint8_t tmp[trie->ngrams[n-1]->elem_size / 8 + 1];
    if (n == 1) {
        void *elems[] = { &ngram->probability, &ngram->context_id };
        const int sizes[] = { sizeof(float) * 8, ceil_log2(trie->n_ngrams[n]) };
        elems_compact(elems, tmp, sizes, 2);
    } else {
        void *elems[] = { &ngram->probability, &ngram->word_id, &ngram->context_id };
        const int sizes[] = { sizeof(float) * 8, trie->word_id_size,
                              ceil_log2((n < trie->n) ? trie->n_ngrams[n] : trie->n_ngrams[n-2]) };
        elems_compact(elems, tmp, sizes, 3);
    }
    array_set(trie->ngrams[n-1], at, tmp);
}

static int64_t cmp_ngram(void *a, void *b, void *arg)
{
    struct ngram tmp;
    const int *sizes = (const int*) arg;
    void *dest[] = { &tmp.probability, &tmp.word_id, &tmp.index };
    elem_extract(a, dest, sizes, 3);
    word_id_type a_id = tmp.word_id;
    elem_extract(b, dest, sizes, 3);
    word_id_type b_id = tmp.word_id;
    if (a_id < b_id) return -1;
    else if (a_id > b_id) return 1;
    else return 0;
}

static uint64_t get_context_id(const word_id_type *context_words_ids, const int context_len, const struct trie *trie)
{
    uint64_t index = context_words_ids[0];
    for (int i = 1; i < context_len; i++) {
        struct ngram ngram = get_ngram(trie, i, index);
        struct ngram adjacent_ngram = get_ngram(trie, i, index + 1);
        uint64_t left_index = ngram.index;
        uint64_t right_index = adjacent_ngram.index;
        const int sizes[] = { sizeof(float) * 8, trie->word_id_size, ceil_log2(trie->n_ngrams[i+1]) };
        struct ngram key = { 0, context_words_ids[i], 0};
        if (array_bsearch_r_within(&key, trie->ngrams[i], cmp_ngram, (void *) sizes,
                                   left_index, right_index, &index) != 0) {
            fprintf(stderr, "word id %d not found in [%lu, %lu] slice of %d-gram array", context_words_ids[i],
                    left_index, right_index, i + 1);
            exit(EXIT_FAILURE);
        }
    }
    return index;
}

struct ngram query(const struct trie *trie, char const **words, const int n)
{
    uint64_t index = get_word_id(words[0], trie);
    for (int i = 1; i < n; i++) {
        struct ngram ngram = get_ngram(trie, i, index);
        struct ngram adjacent_ngram = get_ngram(trie, i, index + 1);
        uint64_t left_index = ngram.index;
        uint64_t right_index = adjacent_ngram.index;
        const int sizes[] = { sizeof(float) * 8, trie->word_id_size, ceil_log2(trie->n_ngrams[i+1]) };
        struct ngram key = { 0, get_word_id(words[i], trie), 0};
        if (array_bsearch_r_within(&key, trie->ngrams[i], cmp_ngram, (void *) sizes,
                                   left_index, right_index, &index) != 0) {
            fprintf(stderr, "word id %d not found in [%lu, %lu] slice of %d-gram array", get_word_id(words[i], trie),
                    left_index, right_index, i + 1);
            exit(EXIT_FAILURE);
        }
    }
    return get_ngram(trie, n, index);
}