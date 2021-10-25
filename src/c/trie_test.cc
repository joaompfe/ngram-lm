// Copyright (c) 2021, João Fé, All rights reserved.

extern "C" {
#include <unistd.h>
#include "c/trie.h"
#include "c/ngram.h"
#include "c/arpa.h"
}

#include <gtest/gtest.h>
#include <fstream>

const char *TEST_DATA = "./data/tmp.arpa";

static void validate_trie(const struct trie *t)
{
    EXPECT_EQ(t->order, 3);

    EXPECT_EQ(t->n_ngrams[0], 209);
    EXPECT_EQ(t->n_ngrams[1], 323);
    EXPECT_EQ(t->n_ngrams[2], 325);

    for (int i = 1; i < t->n_ngrams[0]; i++) {
        EXPECT_TRUE(
                t->vocab_lookup[i - 1].hash < t->vocab_lookup[i].hash);
    }

    EXPECT_EQ(t->arrays[0]->len, 210);
    EXPECT_EQ(t->arrays[1]->len, 324);
    EXPECT_EQ(t->arrays[2]->len, 325);
}

TEST(Trie, trie_new_from_arpa)
{
    struct trie *t = trie_new_from_arpa(3, arpa_open(TEST_DATA));
    validate_trie(t);
    trie_delete(t);
}

TEST(Trie, trie_new_from_arpa_path)
{
    struct trie *t = trie_new_from_arpa_path(3, TEST_DATA);
    validate_trie(t);
    trie_delete(t);
}

TEST(Trie, trie_query_ngram)
{
    struct trie *t = trie_new_from_arpa(3, arpa_open(TEST_DATA));
    char const *grams[10];
    grams[0] = "caso";
    grams[1] = "português";
    int n = 2;
    struct ngram *ngram = trie_query_ngram(t, (char const **) grams, &n);
    EXPECT_EQ(-0.29952f, ngram->probability);
    EXPECT_STREQ(ngram->word->text, "português");

    grams[0] = "garanta";
    grams[1] = "essa";
    grams[2] = "circulação";
    n = 3;
    ngram = trie_query_ngram(t, (char const **) grams, &n);
    EXPECT_STREQ(ngram->word->text, "circulação");
    EXPECT_TRUE(ngram->context != nullptr);
    EXPECT_TRUE(ngram->context->context != nullptr);

    grams[0] = "havia";
    grams[1] = "é";
    n = 2;
    ngram = trie_query_ngram(t, (char const **) grams, &n);
    EXPECT_STREQ(ngram->word->text, "é");
    EXPECT_TRUE(ngram->context == nullptr);

    trie_delete(t);
}

const char *OUT_PATH = "./data/tmp.bin";

TEST(Trie, trie_fwrite_and_trie_fread)
{
    struct trie *t = trie_new_from_arpa(3, arpa_open(TEST_DATA));
    FILE *f = fopen(OUT_PATH, "wb");
    ASSERT_TRUE(f != nullptr) << "Error while opening " << OUT_PATH;

    trie_fwrite(t, f);
    trie_delete(t);
    fclose(f);
    f = fopen(OUT_PATH, "rb");
    ASSERT_TRUE(f != nullptr);
    fclose(f);

    f = fopen(OUT_PATH, "rb");
    size_t read = trie_fread(&t, f);
    EXPECT_EQ(read, 1);
    fclose(f);
    validate_trie(t);
    trie_delete(t);

    std::remove(OUT_PATH);
}

TEST(Trie, trie_save_and_trie_load)
{
    struct trie *t = trie_new_from_arpa(3, arpa_open(TEST_DATA));
    trie_save(t, OUT_PATH);
    FILE *f = fopen(OUT_PATH, "rb");
    ASSERT_TRUE(f != nullptr);
    fclose(f);
    trie_delete(t);

    trie_load(OUT_PATH, &t);
    validate_trie(t);
    trie_delete(t);

    std::remove(OUT_PATH);
}

TEST(Trie, trie_get_word_id_from_text)
{
    struct trie *t = trie_new_from_arpa(3, arpa_open(TEST_DATA));

    EXPECT_EQ(trie_get_word_id_from_text(t, "»"), 0);
    EXPECT_EQ(trie_get_word_id_from_text(t, "amarrar"), 1);
    EXPECT_EQ(trie_get_word_id_from_text(t, "afinal"), 200);

    trie_delete(t);
}

TEST(Trie, trie_word_textncpy)
{
    struct trie *t = trie_new_from_arpa(3, arpa_open(TEST_DATA));
    char word[48];

    EXPECT_STREQ("»", trie_word_textncpy(t, 0, word, 48));
    EXPECT_STREQ("amarrar", trie_word_textncpy(t, 1, word, 48));
    EXPECT_STREQ("afinal", trie_word_textncpy(t, 200, word, 48));

    trie_delete(t);
}

TEST(Trie, trie_get_nwp)
{
    struct trie *t = trie_new_from_arpa(3, arpa_open(TEST_DATA));
    const char *words[] = { "que", "é" };
    struct word *nwp = trie_get_nwp(t, words, 1);
    ASSERT_STREQ(nwp->text, "os");

    words[0] = "é";
    nwp = trie_get_nwp(t, words, 1);
    ASSERT_STREQ(nwp->text, "que");

    words[0] = "<s>";
    nwp = trie_get_nwp(t, words, 1);
    ASSERT_STREQ(nwp->text, "Para");

    nwp = trie_get_nwp(t, words, 0);
    ASSERT_STREQ(nwp->text, "Para");

    words[0] = "anonexistingword";
    nwp = trie_get_nwp(t, words, 1);
    ASSERT_STREQ(nwp->text, "Para");

    words[0] = "Para";
    words[1] = "anonexistingword";
    nwp = trie_get_nwp(t, words, 2);
    ASSERT_STREQ(nwp->text, "Para");

    words[0] = "Para";
    words[1] = "é";
    nwp = trie_get_nwp(t, words, 2);
    ASSERT_STREQ(nwp->text, "que");

    words[0] = "havia";
    words[1] = "é";
    nwp = trie_get_nwp(t, words, 2);
    ASSERT_STREQ(nwp->text, "que");

    trie_delete(t);
}

TEST(Trie, trie_get_k_nwp)
{
    struct trie *t = trie_new_from_arpa(3, arpa_open(TEST_DATA));
    const char *words[5];
    words[0] = "é";
    words[1] = "que";
    struct word *word_preds[10];
    int k = 10;
    trie_get_k_nwp(t, words, 2, k, word_preds);
    ASSERT_STREQ(word_preds[0]->text, "os");
    ASSERT_STREQ(word_preds[1]->text, "levaram");
    ASSERT_STREQ(word_preds[2]->text, "já");
    ASSERT_STREQ(word_preds[3]->text, "avançaram");
    ASSERT_STREQ(word_preds[4]->text, "Público");
    ASSERT_STREQ(word_preds[5]->text, "dentro");
    ASSERT_STREQ(word_preds[6]->text, "lhes");
    ASSERT_STREQ(word_preds[7]->text, "reforça");
    ASSERT_STREQ(word_preds[8]->text, "Na");
    ASSERT_STREQ(word_preds[9]->text, "Em");

    words[0] = "é";
    words[1] = "que";
    words[2] = "que";
    EXPECT_EXIT(
            trie_get_k_nwp(t, words, 3, k, word_preds);,
            ::testing::KilledBySignal(11),
            ".*"
    );

    trie_delete(t);
}
