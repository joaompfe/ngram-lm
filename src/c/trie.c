// Copyright (c) 2021, João Fé, All rights reserved.

#include "trie.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <errno.h>

#include "array.h"
#include "c/util/murmur3.h"
#include "ngram.h"
#include "util/log.h"
#include "util/progress.h"
#include "word.h"

#define ceil_log2(x) ((uint8_t) ceil(log2(x)))
#define WORD_MAX_LENGTH 256
#define KNOWN_PORTUGUESE_WORD_MAX_LENGTH 46

/**
 * Used during trie creation.
 */
struct array_tmp_record {
    float probability;
    word_id_type word_id;
    uint64_t context_id;
};

static struct trie *trie_new(unsigned short order);

static void
read_n_ngrams(int order, const struct arpa *arpa, uint64_t *n_ngrams);

static void create_vocab_lookup(uint64_t n_unigrams, const struct arpa *arpa,
                                struct trie *t);

static int cmp_words(const void *a, const void *b);

static void populate_ngrams(int order, const struct arpa *arpa, struct trie *t);

static void populate_unigrams(const struct arpa *arpa, struct trie *t);

static int cmp_array_tmp_records(void *a, void *b, void *arg);

static void
parse_ngram_definition(const char *line, int n, const struct trie *trie,
                       struct array_tmp_record *tmp_ngram);

static inline int is_unknown_wid(const struct trie *t, word_id_type id);

static void set_array_record(const struct trie *t, int n, uint64_t at,
                             struct array_record *ngram);

static struct array_record
get_array_record(const struct trie *t, int n, uint64_t at);

static int cmp_array_records(void *a, void *b, void *arg);

static uint64_t
get_context_id(const struct trie *t, const word_id_type *context_words_ids,
               int context_len);

static struct array_tmp_record
get_array_tmp_record(const struct trie *t, int n, uint64_t at);

static void set_array_tmp_record(const struct trie *t, int n, uint64_t at,
                                 struct array_tmp_record *tmp_record);

static void fill_in_array_record_indexes(const struct trie *t, int n);

static void reduce_last_order_array(struct trie *t);

static unsigned long get_array_record_size(const struct trie *t, int n);

static unsigned long get_array_tmp_record_size(const struct trie *t, int n);

static void
get_array_record_field_sizes(const struct trie *t, int n, unsigned int *dest);

static void get_array_tmp_record_field_sizes(const struct trie *t, int n,
                                             unsigned int *dest);

static const struct word *
trie_get_word_from_text(const struct trie *t, const char *word_text);

static struct trie *trie_new(unsigned short order)
{
    struct trie *t = malloc(sizeof(struct trie));
    t->order = order;
    t->n_ngrams = malloc(order * sizeof(int));
    t->arrays = malloc(order * sizeof(struct array *));
    return t;
}

struct trie *trie_new_from_arpa(unsigned short order, const struct arpa *arpa)
{
    struct trie *t = trie_new(order);
    read_n_ngrams(order, arpa, t->n_ngrams);
    create_vocab_lookup(t->n_ngrams[0], arpa, t);
    populate_ngrams(order, arpa, t);
    return t;
}

struct trie *
trie_new_from_arpa_path(unsigned short order, const char *arpa_path)
{
    struct arpa *arpa = arpa_open(arpa_path);
    struct trie *t = trie_new_from_arpa(order, arpa);
    arpa_close(arpa);
    return t;
}

void trie_delete(struct trie *t)
{
    for (int i = 0; i < t->order; i++) {
        array_delete(t->arrays[i]);
    }
    free(t->arrays);
    free(t->vocab_lookup);
    free(t->n_ngrams);
    free(t);
}

void trie_fwrite(const struct trie *t, FILE *f)
{
    fwrite(t, sizeof(struct trie), 1, f);
    fwrite(t->n_ngrams, sizeof(uint64_t), t->order, f);
    fwrite(t->vocab_lookup, sizeof(struct word), t->n_ngrams[0], f);
    for (unsigned int i = 0; i < t->n_ngrams[0]; i++) {
        char *word = t->vocab_lookup[i].text;
        unsigned int len = strlen(word);
        fwrite(&len, sizeof(unsigned int), 1, f);
        fwrite(word, sizeof(char), len, f);
    }
    for (int i = 0; i < t->order; i++) {
        array_fwrite(t->arrays[i], f);
    }
}

size_t trie_fread(struct trie **trie, FILE *f)
{
    struct trie *t = malloc(sizeof(struct trie));
    size_t read = fread(t, sizeof(struct trie), 1, f);
    if (read != 1) {
        log_error("Could not read struct trie from file");
        return read;
    }
    t->n_ngrams = malloc(t->order * sizeof(uint64_t));
    read = fread(t->n_ngrams, sizeof(uint64_t), t->order, f);
    if (read != t->order) {
        log_error("Could not read trie->n_ngrams from file");
        return read;
    }
    t->vocab_lookup = malloc(t->n_ngrams[0] * sizeof(struct word));
    read = fread(t->vocab_lookup, sizeof(struct word), t->n_ngrams[0], f);
    if (read != t->n_ngrams[0]) {
        log_error("Could not read vocabulary lookup from file");
        return read;
    }
    for (unsigned int i = 0; i < t->n_ngrams[0]; i++) {
        unsigned int len;
        read = fread(&len, sizeof(unsigned int), 1, f);
        if (read != 1) {
            log_error("Could not read length of word");
            return read;
        }
        char *word = malloc((len + 1) * sizeof(char));
        read = fread(word, sizeof(char), len, f);
        if (read != len) {
            log_error("Could not read word text");
            return read;
        }
        word[len] = '\0';
        t->vocab_lookup[i].text = word;
    }
    t->arrays = malloc(t->order * sizeof(struct array *));
    for (int i = 0; i < t->order; i++) {
        t->arrays[i] = malloc(t->n_ngrams[i] * get_array_record_size(t, i + 1));
        read = array_fread(t->arrays[i], f);
        if (read != 1) {
            log_error("Could not read trie->arrays[%d] from file", i);
            return read;
        }
    }
    *trie = t;
    return 1;
}

int trie_save(const struct trie *t, const char *path)
{
    FILE *f = fopen(path, "wb");
    if (f == NULL) {
        log_warn("'%s' file could not be opened: %s", path, strerror(errno));
        return 1;
    }
    trie_fwrite(t, f);
    fclose(f);
    return 0;
}

int trie_load(const char *path, struct trie **t)
{
    FILE *f = fopen(path, "rb");
    if (f == NULL) {
        log_warn("'%s' file could not be opened: %s", path, strerror(errno));
        return 1;
    }
    trie_fread(t, f);
    fclose(f);
    return 0;
}

static void
read_n_ngrams(const int order, const struct arpa *arpa, uint64_t *n_ngrams)
{
    for (int i = 0; i < order; i++)
        n_ngrams[i] = arpa->n_ngrams[i];
}

static int
create_vocab_lookup_action_f(struct arpa_ngram *ngram, uint64_t i, void *arg)
{
    struct trie *t = arg;
    const char *word = ngram->words[0];
    if (strlen(word) > KNOWN_PORTUGUESE_WORD_MAX_LENGTH) {
        log_warn(
                "A text with %lu characters was found at the %lu-th line of the 1-grams section",
                strlen(word), i + 1);
    }
    uint64_t out[2];
    murmurhash3(word, strlen(word), out);
    word_hash_type hash = out[0]; // qhashmurmur3_32(word, strlen(word));
    t->vocab_lookup[i].hash = hash;
    t->vocab_lookup[i].text = malloc((strlen(word) + 1) * sizeof(char));
    strcpy(t->vocab_lookup[i].text, word);
    return 0;
}

static void
create_vocab_lookup(const uint64_t n_unigrams, const struct arpa *arpa,
                    struct trie *t)
{
    t->vocab_lookup = malloc(n_unigrams * sizeof(struct word));
    struct arpa_section *unigrams_section = arpa_get_section(arpa, 1);
    arpa_for_each_section_ngrami(unigrams_section, create_vocab_lookup_action_f,
                                 t);
    qsort(t->vocab_lookup, n_unigrams, sizeof(struct word), cmp_words);
}

static int cmp_words(const void *a, const void *b)
{
    struct word *a_entry = (struct word *) a, *b_entry = (struct word *) b;
    if (a_entry->hash < b_entry->hash) return -1;
    else if (a_entry->hash > b_entry->hash) return 1;
    else return 0;
}

static int populate_ngrams_action_f(char *line, uint64_t i, void *arg)
{
    if (line[0] == '\n')
        return 1;
    void **args = arg;
    int n = *(int *) args[0];
    struct trie *t = (struct trie *) args[1];
    struct array_tmp_record tmp;
    tmp.context_id = 0;
    tmp.probability = 0;
    tmp.word_id = 0;
    parse_ngram_definition(line, n, t, &tmp);
    set_array_tmp_record(t, n, i, &tmp);
    progress_bar("Reading ARPA", i, t->n_ngrams[n - 1]);
    return 0;
}

static void populate_ngrams(int order, const struct arpa *arpa, struct trie *t)
{
    populate_unigrams(arpa, t);
    for (int n = 2; n <= order; n++) {
        log_info("Populating %d-grams", n);

        t->arrays[n - 1] = array_new(get_array_tmp_record_size(t, n),
                                     t->n_ngrams[n - 1] + 1);
        log_info("Array allocated");

        char section_tile[24];
        snprintf(section_tile, 24, "\\%d-grams:\n", n);
        void *args[] = { &n, t };
        struct arpa_section *arpa_section = arpa_get_section(arpa, n);
        uint64_t i = arpa_for_each_section_linei(arpa_section,
                                                 populate_ngrams_action_f,
                                                 args);
        struct array_tmp_record dummy = { 0, t->n_ngrams[0],
                                          t->n_ngrams[n - 2] };
        set_array_tmp_record(t, n, i, &dummy);

        log_info("Sorting... This might take a while...");
        unsigned int tmp_sizes[3];
        get_array_tmp_record_field_sizes(t, n, tmp_sizes);
        array_sort_r(t->arrays[n - 1], cmp_array_tmp_records,
                     (void *) tmp_sizes);

        fill_in_array_record_indexes(t, n - 1);
    }
    reduce_last_order_array(t);
}

static int
populate_unigrams_action_f(struct arpa_ngram *ngram, uint64_t i, void *arg)
{
    struct unigram {
        float probability;
        uint64_t index;
    } unigram;
    char *word = ngram->words[0];
    struct trie *t = arg;

    word_id_type id = trie_get_word_id_from_text(t, word);
    unigram.probability = ngram->probability;
    unigram.index = 0;
    array_set(t->arrays[0], id, &unigram);
    progress_bar("Reading ARPA", i, t->n_ngrams[0]);
    return 0;
}

static void populate_unigrams(const struct arpa *arpa, struct trie *t)
{
    log_info("Populating 1-grams");
    t->arrays[0] = array_new(get_array_record_size(t, 1), t->n_ngrams[0] + 1);
    log_info("Array allocated");
    struct arpa_section *s = arpa_get_section(arpa, 1);
    arpa_for_each_section_ngrami(s, populate_unigrams_action_f, t);
}

static void reduce_last_order_array(struct trie *t)
{
    const int n = t->order;
    struct array *old = t->arrays[n - 1];
    struct array *new = array_new(get_array_record_size(t, t->order),
                                  t->n_ngrams[n - 1]);
    for (unsigned int i = 0; i < t->n_ngrams[n - 1]; i++) {
        t->arrays[n - 1] = old;
        struct array_tmp_record ngram = get_array_tmp_record(t, n, i);
        t->arrays[n - 1] = new;
        set_array_record(t, n, i, (struct array_record *) &ngram);
        progress_bar("Reducing N-gram array", i, t->n_ngrams[n - 1]);
    }
    array_delete(old);
}

static void fill_in_array_record_indexes(const struct trie *t, int n)
{
    n++;
    struct array_record dummy = get_array_record(t, n - 1, t->n_ngrams[n - 2]);
    dummy.first_child_index = t->n_ngrams[n - 1];
    set_array_record(t, n - 1, t->n_ngrams[n - 2], &dummy);

    uint64_t context_id = get_array_tmp_record(t, n, 0).context_id;
    struct array_record parent_ngram = get_array_record(t, n - 1, context_id);
    parent_ngram.first_child_index = 0;
    set_array_record(t, n - 1, context_id, &parent_ngram);
    for (unsigned int i = 0; i < t->n_ngrams[n - 1]; i++) {
        while (context_id < get_array_tmp_record(t, n, i).context_id) {
            context_id++;
            parent_ngram = get_array_record(t, n - 1, context_id);
            parent_ngram.first_child_index = i;
            set_array_record(t, n - 1, context_id, &parent_ngram);
        }
        progress_bar("Filling in the indexes", i, t->n_ngrams[n - 1]);
    }
}

static int cmp_array_tmp_records(void *a, void *b, void *arg)
{
    struct array_tmp_record tmp;
    const unsigned int *sizes = (const unsigned int *) arg;
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

static void
parse_ngram_definition(const char *line, const int n, const struct trie *trie,
                       struct array_tmp_record *tmp_ngram)
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

        ids[i] = trie_get_word_id_from_text(trie, word);
    }
    tmp_ngram->context_id = get_context_id(trie, ids, n - 1);
    tmp_ngram->word_id = ids[n - 1];
}

word_id_type
trie_get_word_id_from_text(const struct trie *t, const char *word_text)
{
    uint64_t out[2];
    murmurhash3(word_text, strlen(word_text), out);
    word_hash_type hash = out[0]; // qhashmurmur3_32(word_text, strlen
    // (word_text));
    struct word key = { hash, (char *) word_text };
    void *idx = bsearch(&key, t->vocab_lookup, t->n_ngrams[0],
                        sizeof(struct word), cmp_words);
    word_id_type id =
            ((intptr_t) idx - (intptr_t) t->vocab_lookup) / sizeof(struct word);
    if (is_unknown_wid(t, id)) {
        log_warn("'%s' text is not listed in the vocabulary lookup", word_text);
        return -1;
    }
    return id;
}

static inline int is_unknown_wid(const struct trie *t, word_id_type id)
{
    return id >= t->n_ngrams[0];
}

static void
trie_get_word_ids(const struct trie *t, const char **word_text, unsigned int n,
                  word_id_type *dest)
{
    for (unsigned int i = 0; i < n; i++)
        dest[i] = trie_get_word_id_from_text(t, word_text[i]);
}

static void set_array_record(const struct trie *t, int n, uint64_t at,
                             struct array_record *ngram)
{
    uint8_t tmp[t->arrays[n - 1]->elem_size / 8 + 1];
    unsigned int sizes[3];
    get_array_record_field_sizes(t, n, sizes);
    if (n == 1) {
        void *elems[] = { &ngram->probability, &ngram->first_child_index };
        elems_compact(elems, tmp, sizes, 2);
    } else if (n < t->order) {
        void *elems[] = { &ngram->probability, &ngram->word_id,
                          &ngram->first_child_index };
        elems_compact(elems, tmp, sizes, 3);
    } else {
        void *elems[] = { &ngram->probability, &ngram->word_id };
        elems_compact(elems, tmp, sizes, 2);
    }
    array_set(t->arrays[n - 1], at, tmp);
}

static struct array_record
get_array_record(const struct trie *t, int n, uint64_t at)
{
    struct array_record ngram = { 0, 0, 0 };
    unsigned int sizes[3];
    get_array_record_field_sizes(t, n, sizes);
    if (n == 1) {
        void *dest[] = { &ngram.probability, &ngram.first_child_index };
        array_get_extracted(t->arrays[n - 1], at, dest, sizes, 2);
    } else if (n < t->order) {
        void *dest[] = { &ngram.probability, &ngram.word_id,
                         &ngram.first_child_index };
        array_get_extracted(t->arrays[n - 1], at, dest, sizes, 3);
    } else {
        void *dest[] = { &ngram.probability, &ngram.word_id };
        array_get_extracted(t->arrays[n - 1], at, dest, sizes, 2);
    }
    return ngram;
}

static struct array_tmp_record
get_array_tmp_record(const struct trie *t, int n, uint64_t at)
{
    struct array_tmp_record ngram = { 0, 0, 0 };
    unsigned int sizes[3];
    get_array_tmp_record_field_sizes(t, n, sizes);
    if (n == 1) {
        void *dest[] = { &ngram.probability, &ngram.context_id };
        array_get_extracted(t->arrays[n - 1], at, dest, sizes, 2);
    } else {
        void *dest[] = { &ngram.probability, &ngram.word_id,
                         &ngram.context_id };
        array_get_extracted(t->arrays[n - 1], at, dest, sizes, 3);
    }
    return ngram;
}

static void set_array_tmp_record(const struct trie *t, int n, uint64_t at,
                                 struct array_tmp_record *tmp_record)
{
    uint8_t tmp[t->arrays[n - 1]->elem_size / 8 + 1];
    unsigned int sizes[3];
    get_array_tmp_record_field_sizes(t, n, sizes);
    if (n == 1) {
        void *elems[] = { &tmp_record->probability, &tmp_record->context_id };
        elems_compact(elems, tmp, sizes, 2);
    } else {
        void *elems[] = { &tmp_record->probability, &tmp_record->word_id,
                          &tmp_record->context_id };
        elems_compact(elems, tmp, sizes, 3);
    }
    array_set(t->arrays[n - 1], at, tmp);
}

static int cmp_array_records(void *a, void *b, void *arg)
{
    struct array_record tmp;
    const unsigned int *sizes = (const unsigned int *) arg;
    void *dest[] = { &tmp.probability, &tmp.word_id, &tmp.first_child_index };
    elem_extract(a, dest, sizes, 3);
    word_id_type a_id = tmp.word_id;
    elem_extract(b, dest, sizes, 3);
    word_id_type b_id = tmp.word_id;
    if (a_id < b_id) return -1;
    else if (a_id > b_id) return 1;
    else return 0;
}

static unsigned short
map_trie_path(const struct trie *t, const word_id_type *word_ids,
              unsigned short path_len,
              void (*f)(struct array_record *ar, uint64_t ar_index,
                        unsigned short trie_level, void *arg), void *arg)
{
    unsigned short n = 1;
    uint64_t index = word_ids[0];
    struct array_record ar = get_array_record(t, 1, index);
    ar.word_id = index;
    f(&ar, index, 1, arg);
    for (; n < path_len; n++) {
        struct array_record adjacent_ar = get_array_record(t, n, index + 1);
        uint64_t left_index = ar.first_child_index;
        uint64_t right_index = adjacent_ar.first_child_index;
        unsigned int sizes[3];
        get_array_record_field_sizes(t, n + 1, sizes);
        struct array_record key = { 0, word_ids[n], 0 };
        if (array_bsearch_r_within(&key, t->arrays[n], cmp_array_records,
                                   (void *) sizes, left_index, right_index,
                                   &index) != 0) {
            break;
        }
        ar = get_array_record(t, n + 1, index);
        f(&ar, index, n + 1, arg);
    }
    return n;
}

static void get_context_id_f(struct array_record *ar, uint64_t ar_index,
                             unsigned short trie_level, void *arg)
{
    uint64_t *indexes = arg;
    indexes[trie_level - 1] = ar_index;
}

static uint64_t
get_context_id(const struct trie *t, const word_id_type *context_words_ids,
               int context_len)
{
    uint64_t indexes[context_len];
    unsigned short n;
    if ((n = map_trie_path(t, context_words_ids, context_len, get_context_id_f,
                           indexes)) != context_len) {
        log_error("Trie path could only reach %d nodes", n);
        exit(EXIT_FAILURE);
    }
    return indexes[context_len - 1];
}

static void trie_query_ngram_f(struct array_record *ar, uint64_t ar_index,
                               unsigned short trie_level, void *arg)
{
    void **args = arg;
    struct trie *t = args[0];
    struct ngram **ngrams = args[1];
    struct ngram *ngram = ngrams[trie_level - 1];
    ngram->probability = ar->probability;
    ngram->backoff = -1.0f;
    ngram->word = &t->vocab_lookup[ar->word_id];
}

struct ngram *trie_query_ngram(const struct trie *t, char const **words, int *n)
{
    word_id_type ids[*n];
    for (int i = 0; i < *n; i++) {
        ids[i] = trie_get_word_id_from_text(t, words[i]);
    }
    struct ngram *ngrams[*n];
    for (int i = 0; i < *n; i++) {
        ngrams[i] = ngram_new_empty_unigram();
        if (i > 0)
            ngrams[i]->context = ngrams[i - 1];
    }
    void *args[] = { (void *) t, (void *) ngrams };
    int ids_start = 0;
    while (map_trie_path(t, &ids[ids_start], *n, trie_query_ngram_f, args) <
           *n) {
        ids_start++;
        args[1] = &ngrams[ids_start];
        *n = *n - 1;
    }

    for (int i = 0; i < ids_start; i++)
        ngram_delete(ngrams[i]);
    ngrams[ids_start]->context = NULL;
    return ngrams[ids_start + *n - 1];
}

static unsigned long get_array_record_size(const struct trie *t, int n)
{
    if (n == 1)
        return 8 * (sizeof(float)) + ceil_log2(t->n_ngrams[n] + 1);
    if (n == t->order)
        return 8 * sizeof(float) + ceil_log2(t->n_ngrams[0]);
    return 8 * (sizeof(float)) + ceil_log2(t->n_ngrams[0]) +
           ceil_log2(t->n_ngrams[n] + 1);
}

static unsigned long get_array_tmp_record_size(const struct trie *t, int n)
{
    if (n == 1)
        return 8 * (sizeof(float)) + ceil_log2(t->n_ngrams[n] + 1);
    if (n == t->order)
        return 8 * (sizeof(float)) + ceil_log2(t->n_ngrams[0]) +
               ceil_log2(t->n_ngrams[n - 2] + 1);
    return 8 * (sizeof(float)) + ceil_log2(t->n_ngrams[0]) +
           ceil_log2(t->n_ngrams[n] + 1);
}

static void
get_array_record_field_sizes(const struct trie *t, int n, unsigned int *dest)
{
    dest[0] = 8 * sizeof(float);
    dest[1] = (n == 1) ? ceil_log2(t->n_ngrams[n] + 1) :
              ceil_log2(t->n_ngrams[0]);
    if (n != 1 && n != t->order)
        dest[2] = ceil_log2(t->n_ngrams[n] + 1);
}

static void get_array_tmp_record_field_sizes(const struct trie *t, int n,
                                             unsigned int *dest)
{
    dest[0] = 8 * sizeof(float);
    dest[1] = (n == 1) ? ceil_log2(t->n_ngrams[n] + 1) : ceil_log2
    (t->n_ngrams[0]);
    if (n != 1)
        dest[2] = (n == t->order) ? ceil_log2(t->n_ngrams[n - 2] + 1) :
                  ceil_log2(t->n_ngrams[n] + 1);
}

char *
trie_word_textncpy(const struct trie *t, word_id_type id, char *dest, size_t n)
{
    return strncpy(dest, t->vocab_lookup[id].text, n);
}

static const struct word *
trie_get_word_from_text(const struct trie *t, const char *word_text)
{
    word_id_type id = trie_get_word_id_from_text(t, word_text);
    return &t->vocab_lookup[id];
}

static void
trie_get_words_from_text(const struct trie *t, const char **word_text,
                         unsigned int n,
                         const struct word **dest)
{
    for (unsigned int i = 0; i < n; i++) {
        dest[i] = trie_get_word_from_text(t, word_text[i]);
    }
}

static void trie_get_nwp_f(struct array_record *ar, uint64_t ar_index,
                           unsigned short trie_level, void *arg)
{
    uint64_t *index = arg;
    *index = ar_index;
}

struct word *trie_get_nwp(const struct trie *t, const char **words, int n)
{
    int is_nwp_for_sentence_start = n == 0;
    uint64_t index;
    word_id_type ids[n];
    int ids_start = 0;

    if (!is_nwp_for_sentence_start) {
        trie_get_word_ids(t, words, n, ids);
        for (int i = 0; i < n; i++)
            if (is_unknown_wid(t, ids[i]))
                ids_start = i + 1;
        n = n - ids_start;
        is_nwp_for_sentence_start = n == 0;
    }

    if (!is_nwp_for_sentence_start)
        while (map_trie_path(t, &ids[ids_start], n, trie_get_nwp_f, &index) <
               n) {
            ids_start++;
            n--;
        }
    else {
        index = trie_get_word_id_from_text(t, "<s>");
        n = 1;
    }

    struct array_record ar = get_array_record(t, n, index);
    struct array_record adjacent_ar = get_array_record(t, n, index + 1);
    uint64_t left = ar.first_child_index;
    uint64_t right = adjacent_ar.first_child_index;
    struct array_record nwp_record = get_array_record(t, n + 1, left);;
    for (uint64_t i = left + 1; i < right; i++) {
        struct array_record tmp_nwp_record = get_array_record(t, n + 1, i);
        if (tmp_nwp_record.probability > nwp_record.probability)
            nwp_record = tmp_nwp_record;
    }
    return &t->vocab_lookup[nwp_record.word_id];
}

float trie_ngram_probability(const struct trie *t, const char **words, int n)
{
    int tmp_n = n;
    struct ngram *ngram = trie_query_ngram(t, words, &tmp_n);
    if (tmp_n == n)
        return ngram->probability;
    else
        return ngram->probability + ngram->backoff;
}
