from word cimport word_id_type, word

from libc.stdio cimport FILE


cdef extern from "trie.h":
    cdef struct trie:
        pass

    trie *trie_new_from_arpa_path(unsigned short order, const char *arpa_path)
    void trie_delete(trie *t)
    word_id_type trie_get_word_id_from_text(const trie *t, const char *word_text)
    void trie_fwrite(const trie *t, FILE *f)
    int trie_save(const trie *t, const char *path)
    int trie_load(const char *path, trie **t)
    word_id_type trie_get_word_id(const trie *t, const char *word_text)
    word *trie_get_nwp(const trie *t, const char **words, int n)
    void trie_get_k_nwp(const trie *t, const char **words, int n, unsigned short k, word **predictions)
