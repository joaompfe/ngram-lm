// Copyright (c) 2021, João Fé, All rights reserved.

extern "C" {
#include "ngram_trie.h"
#include <unistd.h>
#include <stdio.h>
}

#include <gtest/gtest.h>


TEST(TrieTest, TrieCreation)
{
    struct trie *trie = new_trie_from_arpa("./data/tmp.arpa", 3);
    EXPECT_EQ(trie->n, 3);
    EXPECT_EQ(trie->n_ngrams[0], 209);
    EXPECT_EQ(trie->n_ngrams[1], 323);
    EXPECT_EQ(trie->n_ngrams[2], 325);
}

TEST(TrieTest, VocabLookupArray)
{
    struct trie *trie = new_trie_from_arpa("./data/tmp.arpa", 3);
    for (int i = 1; i < trie->n_ngrams[0]; i++) {
        EXPECT_TRUE(trie->vocab_lookup[i-1] < trie->vocab_lookup[i]);
    }
}

TEST(TrieTest, UnigramArray)
{
    struct trie *trie = new_trie_from_arpa("./data/tmp.arpa", 3);
    struct ngram ngram;
    ngram = get_ngram(trie, 1, get_word_id("Que", trie));
    EXPECT_EQ(-2.45805f, ngram.probability);
    ngram = get_ngram(trie, 1, get_word_id("aviação", trie));
    EXPECT_EQ(-2.45805f, ngram.probability);
    ngram = get_ngram(trie, 1, get_word_id("esses", trie));
    EXPECT_EQ(-2.45805f, ngram.probability);
    ngram = get_ngram(trie, 1, get_word_id(".", trie));
    EXPECT_EQ(-1.5993792f, ngram.probability);
    ngram = get_ngram(trie, 1, get_word_id("mais", trie));
    EXPECT_EQ(-2.0143526f, ngram.probability);
    ngram = get_ngram(trie, 1, get_word_id("europeia", trie));
    EXPECT_EQ(-2.45805f, ngram.probability);
}

TEST(TrieTest, UnigramArrayIndexes)
{
    struct trie *trie = new_trie_from_arpa("./data/tmp.arpa", 3);

    for (int i = 0; i < trie->n_ngrams[0]; i++) {
//        std::cout << i << " IDX--> " << get_ngram(trie, 1, i).index << std::endl;
        EXPECT_TRUE(get_ngram(trie, 1, i).index <= get_ngram(trie, 1, i + 1).index);  // note: there is no bug, it intentionally reaches (trie->n_ngrams[0] + 1)th unigram
    }
//    std::cout << trie->n_ngrams[0] << " IDX--> " << get_ngram(trie, 1, trie->n_ngrams[0]).index << std::endl;

    struct ngram unigram, bigram;
    // there is only one bigram starting with 'assuntos', which is 'assuntos .'
    unigram = get_ngram(trie, 1, get_word_id("assuntos", trie));
    bigram = get_ngram(trie, 2, unigram.index);
    EXPECT_EQ(bigram.word_id, get_word_id(".", trie));
    // there is only one bigram starting with 'crescimento', which is 'crescimento económico'
    unigram = get_ngram(trie, 1, get_word_id("crescimento", trie));
    bigram = get_ngram(trie, 2, unigram.index);
    EXPECT_EQ(bigram.word_id, get_word_id("económico", trie));
    // ther are two bigrams starting with 'é', which are 'é que' and 'é mais'
    unigram = get_ngram(trie, 1, get_word_id("é", trie));
    bigram = get_ngram(trie, 2, unigram.index);
    word_id_type id0 = get_word_id("que", trie), id1 = get_word_id("mais", trie);
    EXPECT_EQ(bigram.word_id, (id0 < id1) ? id0 : id1);
    bigram = get_ngram(trie, 2, unigram.index + 1);
    EXPECT_EQ(bigram.word_id, (id0 > id1) ? id0 : id1);
}

TEST(TrieTest, BigramArrayIndexes)
{
    struct trie *trie = new_trie_from_arpa("./data/tmp.arpa", 3);

    for (int i = 0; i < trie->n_ngrams[1]; i++) {
//        std::cout << i << " IDX--> " << get_ngram(trie, 2, i).index << std::endl;
        EXPECT_TRUE(get_ngram(trie, 2, i).index <= get_ngram(trie, 2, i + 1).index);  // note: there is no bug, it intentionally reaches (trie->n_ngrams[0] + 1)th unigram
    }
//    std::cout << trie->n_ngrams[1] << " IDX--> " << get_ngram(trie, 2, trie->n_ngrams[1]).index << std::endl;

    struct ngram unigram, bigram, trigram;
    // there is only one trigram starting with 'despertar', which is 'despertar foi duro'
    unigram = get_ngram(trie, 1, get_word_id("despertar", trie));
    bigram = get_ngram(trie, 2, unigram.index);
    EXPECT_EQ(bigram.word_id, get_word_id("foi", trie));
    trigram = get_ngram(trie, 3, bigram.index);
    EXPECT_EQ(trigram.word_id, get_word_id("duro", trie));
}

TEST(TrieTest, Query)
{
    struct trie *trie = new_trie_from_arpa("./data/tmp.arpa", 3);
    char const *grams[2];
    grams[0] = "caso";
    grams[1] = "português";
    struct ngram ngram = query(trie, (char const **) grams, 2);
    EXPECT_EQ(-0.29952f, ngram.probability);
    EXPECT_EQ(ngram.word_id, get_word_id("português", trie));
}