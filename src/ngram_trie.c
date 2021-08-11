// Copyright (c) 2021, João Fé, All rights reserved.

#include "ngram_trie.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <qlibc.h>
#include <errno.h>

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
static int cmp_vocab_entries(const void *a, const void *b);
static void populate_ngrams(int order, FILE *body, struct trie *trie);
static void populate_unigrams(int order, FILE *body, struct trie *trie);
static int64_t cmp_tmp_grams(void *a, void *b, void *arg);
static void parse_ngram_definition(const char *line, int n, const struct trie *trie, struct tmp_ngram *tmp_ngram);
static void set_ngram(const struct trie *trie, int n, uint64_t at, struct ngram *ngram);
static int64_t cmp_ngram(void *a, void *b, void *arg);
static uint64_t get_context_id(const word_id_type *context_words_ids, int context_len, const struct trie *trie);
static struct tmp_ngram get_tmp_ngram(const struct trie *trie, int n, uint64_t at);
static void set_tmp_ngram(const struct trie *trie, int n, uint64_t at, struct tmp_ngram *ngram);
static void fill_in_indexes(const struct trie *trie, int n);
static void reduce_last_order_ngram_array(struct trie *trie);
static unsigned long ngram_size(const struct trie *trie, int n);
static unsigned long tmp_ngram_size(const struct trie *trie, int n);
static void ngram_sizes(const struct trie *trie, int n, unsigned int *dest);
static void tmp_ngram_sizes(const struct trie *trie, int n, unsigned int *dest);

void build_trie_from_arpa(const char *arpa_path, unsigned short order, const char *out_path)
{
    struct trie *t = new_trie_from_arpa(arpa_path, order);
    FILE *f = fopen(out_path, "wb");
    if (f == NULL) {
        fprintf(stderr, "Error while opening %s: %s.\n", out_path, strerror(errno));
        exit(EXIT_FAILURE);
    }
    trie_fwrite(t, f);
    fclose(f);
}

struct trie *new_trie_from_arpa(const char *arpa_path, const unsigned short order) {
    FILE *fp = fopen(arpa_path, "r");
    if (fp == NULL) {
        fprintf(stderr, "Error while opening %s: %s.\n", arpa_path, strerror(errno));
        exit(EXIT_FAILURE);
    }

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

void trie_fwrite(const struct trie *t, FILE *f)
{
    fwrite(t, sizeof(struct trie), 1, f);
    fwrite(t->n_ngrams, sizeof(uint64_t), t->n, f);
    fwrite(t->vocab_lookup, sizeof(struct vocab_entry), t->n_ngrams[0], f);
    for (int i = 0; i < t->n_ngrams[0]; i++) {
        char *word = t->vocab_lookup[i].word;
        unsigned int len = strlen(word);
        fwrite(&len, sizeof(unsigned int), 1, f);
        fwrite(word, sizeof(char), len, f);
    }
    for (int i = 0; i < t->n; i++) {
        array_fwrite(t->ngrams[i], f);
    }
}

size_t trie_fread(struct trie *t, FILE *f)
{
    size_t read = fread(t, sizeof(struct trie), 1, f);
    if (read != 1) {
        fprintf(stderr, "Error while reading struct trie from file.\n");
        return read;
    }
    t->n_ngrams = malloc(t->n * sizeof(uint64_t));
    read = fread(t->n_ngrams, sizeof(uint64_t), t->n, f);
    if (read != t->n) {
        fprintf(stderr, "Error while reading trie->n_ngrams from file.\n");
        return read;
    }
    t->vocab_lookup = malloc(t->n_ngrams[0] * sizeof(struct vocab_entry));
    read = fread(t->vocab_lookup, sizeof(struct vocab_entry), t->n_ngrams[0], f);
    for (int i = 0; i < t->n_ngrams[0]; i++) {
        unsigned int len;
        fread(&len, sizeof(unsigned int), 1, f);
        char *word = malloc((len + 1) * sizeof(char));
        fread(word,  sizeof(char), len, f);
        word[len] = '\0';
        t->vocab_lookup[i].word = word;
    }
    if (read != t->n_ngrams[0]) {
        fprintf(stderr, "Error while reading vocabulary lookup from file.\n");
        return read;
    }
    for (int i = 0; i < t->n; i++) {
        t->ngrams[i] = malloc(t->n_ngrams[i] * ngram_size(t, i + 1));
        read = array_fread(t->ngrams[i], f);
        if (read != 1) {
            fprintf(stderr, "Error while reading trie->ngrams[%d] from file.\n", i);
            return read;
        }
    }
    return 1;
}

#define FOR_EACH_SECTION_LINE(sec, title, do) \
char *line = NULL;                       \
size_t len = 0;                          \
if (getline(&line, &len, sec) == -1) exit(EXIT_FAILURE); \
if (strcmp(line, title) != 0) {          \
    fprintf(stderr, "'%s' expected at the beginning of the section, but '%s' was found instead.\n", title, line); \
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
    FOR_EACH_SECTION_LINE(header, "\\data\\\n",
                          if (i < order && sscanf(line, "ngram %*d=%lu", &n_ngrams[i]) == EOF)
            exit(EXIT_FAILURE);
    )
}

static void create_vocab_lookup(const uint64_t n_unigrams, FILE *body, struct trie *trie)
{
    trie->vocab_lookup = malloc(n_unigrams * sizeof(struct vocab_entry));
    char word[WORD_MAX_LENGTH];
    FOR_EACH_SECTION_LINE(body, "\\1-grams:\n",
        double prob;
        if (sscanf(line, "%lf%s%*f", &prob, word) == EOF)
            exit(EXIT_FAILURE);
        if (strlen(word) > KNOWN_PORTUGUESE_WORD_MAX_LENGTH) {
            fprintf(stderr, "a word with %lu characters was found at the %lu-th line of the 1-grams section.\n",
                    strlen(word), i+1);
        }
        word_id_type hash = qhashmurmur3_32(word, strlen(word));
        trie->vocab_lookup[i].id = hash;
        trie->vocab_lookup[i].word = malloc((strlen(word) + 1) * sizeof(char));
        strcpy(trie->vocab_lookup[i].word, word);
    )
    qsort(trie->vocab_lookup, n_unigrams, sizeof(struct vocab_entry), cmp_vocab_entries);
}

static int cmp_vocab_entries(const void *a, const void *b)
{
    struct vocab_entry *a_entry = (struct vocab_entry *)a, *b_entry = (struct vocab_entry *)b;
    if (a_entry->id < b_entry->id) return -1;
    else if (a_entry->id > b_entry->id) return 1;
    else return 0;
}

static void populate_ngrams(const int order, FILE *body, struct trie *trie)
{
    populate_unigrams(order, body, trie);
    for (int n = 2; n <= order; n++) {
        printf("\nPopulating %d-grams", n);

        trie->ngrams[n-1] = new_array(tmp_ngram_size(trie, n), trie->n_ngrams[n-1] + 1);
        printf("\nArray allocated\n");

        char section_tile[24];
        snprintf(section_tile, 24, "\\%d-grams:\n", n);
        FOR_EACH_SECTION_LINE(body, section_tile,
            struct tmp_ngram tmp;
            tmp.context_id = 0;
            tmp.probability = 0;
            tmp.word_id = 0;
            parse_ngram_definition(line, n, trie, &tmp);
            set_tmp_ngram(trie, n, i, &tmp);
            PROGRESS_BAR("Reading ARPA", i, trie->n_ngrams[n-1])
        )
        struct tmp_ngram dummy = {0, trie->n_ngrams[0], trie->n_ngrams[n - 2] }; // TODO shouldn't these be subtracted with 1, won't they overflow on limit situation?
        set_tmp_ngram(trie, n, i, &dummy);

        printf("\nSorting... This might take a while...\n");
        unsigned int tmp_sizes[3];
        tmp_ngram_sizes(trie, n, tmp_sizes);
        array_sort_r(trie->ngrams[n - 1], cmp_tmp_grams, (void *) tmp_sizes);

        fill_in_indexes(trie, n - 1);
    }
    printf("\n");
    reduce_last_order_ngram_array(trie);
}

static void populate_unigrams(const int order, FILE *body, struct trie *trie)
{
    printf("\nPopulating 1-grams");
    trie->ngrams[0] = new_array(ngram_size(trie, 1), trie->n_ngrams[0] + 1);
    printf("\nArray allocated\n");
    struct unigram {
        float probability;
        uint64_t index;
    };
    char word[WORD_MAX_LENGTH];
    FOR_EACH_SECTION_LINE(body, "\\1-grams:\n",
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
    struct array *new = new_array(ngram_size(trie, trie->n), trie->n_ngrams[n-1]);
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
    const unsigned int *sizes = (const unsigned int*) arg;
    void *dest[] = { &tmp.probability, &tmp.word_id, &tmp.context_id };
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
    }
}

static void parse_ngram_definition(const char *line, const int n, const struct trie *trie, struct tmp_ngram *tmp_ngram)
{
    int nchar_matched;
    if (sscanf(line, "%f%n", &tmp_ngram->probability, &nchar_matched) == EOF)
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
    tmp_ngram->context_id = get_context_id(ids, n - 1, trie);
    tmp_ngram->word_id = ids[n - 1];
}

word_id_type get_word_id(const char *word, const struct trie *trie)
{
    word_id_type hash = qhashmurmur3_32(word, strlen(word));
    struct vocab_entry key = { hash, word };
    void *idx = bsearch(&key, trie->vocab_lookup, trie->n_ngrams[0],
                        sizeof(struct vocab_entry), cmp_vocab_entries);
    word_id_type id = (idx - (void *) trie->vocab_lookup) / sizeof(struct vocab_entry);
    if (id > trie->n_ngrams[0]) {
        fprintf(stderr, "'%s' word is not listed in the vocabulary lookup", word);
        exit(EXIT_FAILURE);
    }
    return id;
}

struct ngram get_ngram(const struct trie *trie, const int n, uint64_t at)
{
    struct ngram ngram = { 0, 0, 0 };
    unsigned int sizes[3];
    ngram_sizes(trie, n, sizes);
    if (n == 1) {
        void *dest[] = { &ngram.probability, &ngram.index };
        array_get_extracted(trie->ngrams[n-1], at, dest, sizes, 2);
    } else if (n < trie->n) {
        void *dest[] = { &ngram.probability, &ngram.word_id, &ngram.index };
        array_get_extracted(trie->ngrams[n-1], at, dest, sizes, 3);
    } else {
        void *dest[] = { &ngram.probability, &ngram.word_id };
        array_get_extracted(trie->ngrams[n-1], at, dest, sizes, 2);
    }
    return ngram;
}

static void set_ngram(const struct trie *trie, const int n, uint64_t at, struct ngram *ngram)
{
    uint8_t tmp[trie->ngrams[n-1]->elem_size / 8 + 1];
    unsigned int sizes[3];
    ngram_sizes(trie, n, sizes);
    if (n == 1) {
        void *elems[] = { &ngram->probability, &ngram->index };
        elems_compact(elems, tmp, sizes, 2);
    } else if (n < trie->n) {
        void *elems[] = { &ngram->probability, &ngram->word_id, &ngram->index };
        elems_compact(elems, tmp, sizes, 3);
    } else {
        void *elems[] = { &ngram->probability, &ngram->word_id };
        elems_compact(elems, tmp, sizes, 2);
    }
    array_set(trie->ngrams[n-1], at, tmp);
}

static struct tmp_ngram get_tmp_ngram(const struct trie *trie, const int n, uint64_t at)
{
    struct tmp_ngram ngram = {0, 0, 0 };
    unsigned int sizes[3];
    tmp_ngram_sizes(trie, n, sizes);
    if (n == 1) {
        void *dest[] = { &ngram.probability, &ngram.context_id };
        array_get_extracted(trie->ngrams[n-1], at, dest, sizes, 2);
    } else {
        void *dest[] = { &ngram.probability, &ngram.word_id, &ngram.context_id };
        array_get_extracted(trie->ngrams[n-1], at, dest, sizes, 3);
    }
    return ngram;
}

static void set_tmp_ngram(const struct trie *trie, const int n, uint64_t at, struct tmp_ngram *ngram)
{
    uint8_t tmp[trie->ngrams[n-1]->elem_size / 8 + 1];
    unsigned int sizes[3];
    tmp_ngram_sizes(trie, n, sizes);
    if (n == 1) {
        void *elems[] = { &ngram->probability, &ngram->context_id };
        elems_compact(elems, tmp, sizes, 2);
    } else {
        void *elems[] = { &ngram->probability, &ngram->word_id, &ngram->context_id };
        elems_compact(elems, tmp, sizes, 3);
    }
    array_set(trie->ngrams[n-1], at, tmp);
}

static int64_t cmp_ngram(void *a, void *b, void *arg)
{
    struct ngram tmp;
    const unsigned int *sizes = (const unsigned int*) arg;
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
        unsigned int sizes[3];
        ngram_sizes(trie, i+1, sizes);
        struct ngram key = {0, context_words_ids[i], 0};
        if (array_bsearch_r_within(&key, trie->ngrams[i], cmp_ngram, (void *) sizes,
                                   left_index, right_index, &index) != 0) {
            fprintf(stderr, "word id %d not found in [%lu, %lu] slice of %d-gram array.\n", context_words_ids[i],
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
        unsigned int sizes[3];
        ngram_sizes(trie, i+1, sizes);
        struct ngram key = { 0, get_word_id(words[i], trie), 0};
        if (array_bsearch_r_within(&key, trie->ngrams[i], cmp_ngram, (void *) sizes,
                                   left_index, right_index, &index) != 0) {
            fprintf(stderr, "word id %d not found in [%lu, %lu] slice of %d-gram array.\n", get_word_id(words[i], trie),
                    left_index, right_index, i + 1);
            exit(EXIT_FAILURE);
        }
    }
    return get_ngram(trie, n, index);
}

static unsigned long ngram_size(const struct trie *trie, int n)
{
    if (n == 1)
        return 8 * (sizeof(float)) + ceil_log2(trie->n_ngrams[n]);
    if (n == trie->n)
        return 8 * sizeof(float) + ceil_log2(trie->n_ngrams[0]);
    return 8 * (sizeof(float)) + ceil_log2(trie->n_ngrams[0]) + ceil_log2(trie->n_ngrams[n]);
}

static unsigned long tmp_ngram_size(const struct trie *trie, int n)
{
    if (n == 1)
        return 8 * (sizeof(float)) + ceil_log2(trie->n_ngrams[n]);
    if (n == trie->n)
        return 8 * (sizeof(float)) + ceil_log2(trie->n_ngrams[0]) + ceil_log2(trie->n_ngrams[n-2]);
    return 8 * (sizeof(float)) + ceil_log2(trie->n_ngrams[0]) + ceil_log2(trie->n_ngrams[n]);
}

static void ngram_sizes(const struct trie *trie, int n, unsigned int *dest)
{
    dest[0] = 8 * sizeof(float);
    dest[1] = (n == 1) ? ceil_log2(trie->n_ngrams[n]) : ceil_log2(trie->n_ngrams[0]);
    if (n != 1 && n != trie->n)
        dest[2] = ceil_log2(trie->n_ngrams[n]);
}

static void tmp_ngram_sizes(const struct trie *trie, int n, unsigned int *dest)
{
    dest[0] = 8 * sizeof(float);
    dest[1] = (n == 1) ? ceil_log2(trie->n_ngrams[n]) : ceil_log2(trie->n_ngrams[0]);
    if (n != 1)
        dest[2] = (n == trie->n) ? ceil_log2(trie->n_ngrams[n-2]) : ceil_log2(trie->n_ngrams[n]);
}

char *get_word(const struct trie *trie, word_id_type id, char *dest, size_t n)
{
    return strncpy(dest, trie->vocab_lookup[id].word, n);
}