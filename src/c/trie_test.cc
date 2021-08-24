// Copyright (c) 2021, João Fé, All rights reserved.

extern "C" {
#include <unistd.h>
#include <stdio.h>
#include "c/trie.h"
#include "c/ngram.h"
#include "c/arpa.h"
}

#include <gtest/gtest.h>

TEST(Trie, TrieNumberOfNgrams)
{
    struct trie *trie = trie_new_from_arpa(3, arpa_open("./data/tmp.arpa"));
    EXPECT_EQ(trie->order, 3);
    EXPECT_EQ(trie->n_ngrams[0], 209);
    EXPECT_EQ(trie->n_ngrams[1], 323);
    EXPECT_EQ(trie->n_ngrams[2], 325);
}

TEST(Trie, VocabLookupArray)
{
    struct trie *trie = trie_new_from_arpa(3, arpa_open("./data/tmp.arpa"));
    for (int i = 1; i < trie->n_ngrams[0]; i++) {
        EXPECT_TRUE(
                trie->vocab_lookup[i - 1].hash < trie->vocab_lookup[i].hash);
    }
}

TEST(Trie, QueryNgram)
{
    struct trie *trie = trie_new_from_arpa(3, arpa_open("./data/tmp.arpa"));
    char const *grams[10];
    grams[0] = "caso";
    grams[1] = "português";
    int n = 2;
    struct ngram *ngram = trie_query_ngram(trie, (char const **) grams, &n);
    EXPECT_EQ(-0.29952f, ngram->probability);
    EXPECT_STREQ(ngram->word->text, "português");

    const struct word *w = &trie->vocab_lookup[trie->n_ngrams[0] - 1];
    grams[0] = "garanta";
    grams[1] = "essa";
    grams[2] = "circulação";
    n = 3;
    ngram = trie_query_ngram(trie, (char const **) grams, &n);
    EXPECT_STREQ(ngram->word->text, "circulação");
}

TEST(Trie, LoadAndSaveFromFile)
{
    const char *arpa_path = "./data/tmp.arpa";
    const char *out_path = "./data/tmp.bin";
    FILE *f = fopen(out_path, "r");
    if (f != nullptr) {
        std::remove(out_path);
        fclose(f);
    }
    struct trie *t = trie_new_from_arpa(3, arpa_open(arpa_path));
    f = fopen(out_path, "wb");
    if (f == nullptr) {
        fprintf(stderr, "Error while opening %s: %s.\n", out_path,
                strerror(errno));
        FAIL();
    }
    trie_fwrite(t, f);
    fclose(f);
    f = fopen(out_path, "rb");
    ASSERT_TRUE(f != nullptr);
    fclose(f);

    struct trie *trie;
    f = fopen(out_path, "rb");
    size_t read = trie_fread(&trie, f);
    EXPECT_EQ(read, 1);
    fclose(f);
    EXPECT_EQ(trie->order, 3);
    EXPECT_EQ(trie->n_ngrams[0], 209);
    EXPECT_EQ(trie->n_ngrams[1], 323);
    EXPECT_EQ(trie->n_ngrams[2], 325);
    char const *grams[2];
    grams[0] = "caso";
    grams[1] = "português";
    int n = 2;
    struct ngram *ngram = trie_query_ngram(trie, (char const **) grams, &n);
    EXPECT_EQ(-0.29952f, ngram->probability);
    EXPECT_STREQ(ngram->word->text, "português");
    std::remove(out_path);
}

TEST(Trie, GetWordText)
{
    struct trie *trie = trie_new_from_arpa(3, arpa_open("./data/tmp.arpa"));
    char word[48];
    EXPECT_STREQ("»", trie_word_textncpy(trie, 0, word, 48));
    EXPECT_STREQ("amarrar", trie_word_textncpy(trie, 1, word, 48));
    EXPECT_STREQ("afinal", trie_word_textncpy(trie, 200, word, 48));
}

TEST(Trie, GetNwp)
{
    struct trie *t = trie_new_from_arpa(3, arpa_open("./data/tmp.arpa"));
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

    words[0] = "giaonewnfwpofnjANOFawdWAQ";
    nwp = trie_get_nwp(t, words, 1);
    ASSERT_STREQ(nwp->text, "Para");

    words[0] = "Para";
    words[1] = "giaonewnfwpofnjANOFawdWAQ";
    nwp = trie_get_nwp(t, words, 2);
    ASSERT_STREQ(nwp->text, "Para");
}

TEST(Trie, LoadAndSave)
{
    const char *arpa_path = "./data/tmp.arpa";
    const char *out_path = "./data/tmp.bin";
    FILE *f = fopen(out_path, "r");
    if (f != nullptr) {
        std::remove(out_path);
        fclose(f);
    }
    struct trie *t = trie_new_from_arpa(3, arpa_open(arpa_path));
    trie_save(t, out_path);
    f = fopen(out_path, "rb");
    ASSERT_TRUE(f != nullptr);
    trie_delete(t);
    trie_load(out_path, &t);

    EXPECT_EQ(t->order, 3);
    EXPECT_EQ(t->n_ngrams[0], 209);
    EXPECT_EQ(t->n_ngrams[1], 323);
    EXPECT_EQ(t->n_ngrams[2], 325);
}
