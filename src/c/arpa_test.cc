// Copyright (c) 2021, João Fé, All rights reserved.

extern "C" {
#include "c/arpa.h"
}

#include <gtest/gtest.h>

TEST(Arpa, New)
{
    const char *const path = "data/tmp.arpa";
    struct arpa *a = arpa_open(path);
    ASSERT_TRUE(a != nullptr);
    ASSERT_TRUE(a->f != nullptr);
    ASSERT_STREQ(a->path, path);
    ASSERT_EQ(a->order, 3);
    ASSERT_EQ(a->n_ngrams[0], 209);
    ASSERT_EQ(a->n_ngrams[1], 323);
    ASSERT_EQ(a->n_ngrams[2], 325);
    char *line = nullptr;
    size_t len = 0;
    fsetpos(a->sections[0]->f, a->sections[0]->begin);
    ASSERT_TRUE(getline(&line, &len, a->sections[0]->f) != -1);
    ASSERT_STREQ(line, "\\1-grams:\n");
}

TEST(Arpa, Delete)
{
    const char *const path = "data/tmp.arpa";
    struct arpa *a = arpa_open(path);
    arpa_close(a);
}

TEST(Arpa, GetSection)
{
    const char *const path = "data/tmp.arpa";
    struct arpa *a = arpa_open(path);
    struct arpa_section *s = arpa_get_section(a, 2);
    char *line = nullptr;
    size_t len = 0;
    ASSERT_TRUE(getline(&line, &len, s->f) != -1);
    ASSERT_STREQ(line, "\\2-grams:\n");
    ASSERT_EQ(s->n_ngrams, 323);

    s = arpa_get_section(a, 3);
    line = nullptr;
    len = 0;
    ASSERT_TRUE(getline(&line, &len, s->f) != -1);
    ASSERT_STREQ(line, "\\3-grams:\n");
    ASSERT_EQ(s->n_ngrams, 325);
}

int ngram_map(struct arpa_ngram *a, uint64_t i, void *arg)
{
    if (i < 100 || i == 208) {
        auto *words = static_cast<std::string *>(arg);
        words[i] = a->words[0];
    }
    return 0;
}

TEST(Arpa, ForEachSectionNgram)
{
    const char *const path = "data/tmp.arpa";
    struct arpa *a = arpa_open(path);
    const struct arpa_section *s = arpa_get_section(a, 1);
    std::string words[300];
    arpa_for_each_section_ngrami(s, ngram_map, words);
    ASSERT_STREQ(words[0].c_str(), "<unk>");
    ASSERT_STREQ(words[1].c_str(), "<s>");
    ASSERT_STREQ(words[55].c_str(), "Europeias");
    ASSERT_STREQ(words[80].c_str(), "vida");
    ASSERT_STREQ(words[208].c_str(), "europeia");
    ASSERT_STREQ(words[209].c_str(), "");
}