cimport trie as ctrie

from libc.stdlib cimport malloc, free
from libc.string cimport strcpy
from libc.stdio cimport FILE, fopen, fclose

import multiprocessing

from ngram_lm.word import Word


cdef class Trie:
    cdef ctrie.trie *_c_trie

    def __cinit__(self, trie_path: str = None):
        if trie_path is not None:
            ctrie.trie_load(trie_path.encode(), &self._c_trie)

    @staticmethod
    def from_arpa(order: int, arpa_path: str) -> Trie:
        t = Trie()
        cdef const char *c_arpa_path = <char *> malloc(len(arpa_path) * sizeof(char))
        strcpy(c_arpa_path, arpa_path.encode())
        t._c_trie = ctrie.trie_new_from_arpa_path(order, c_arpa_path)
        free(c_arpa_path)
        return t

    def __dealloc__(self):
        ctrie.trie_delete(self._c_trie)

    def to_disk(self, path: str):
        ctrie.trie_save(self._c_trie, path.encode())

    def from_disk(self, path: str):
        ctrie.trie_load(path.encode(), &self._c_trie)

    def get_word_id(self, word: str):
        return ctrie.trie_get_word_id_from_text(self._c_trie, word.encode())

    def next_word_prediction(self, words: [str]):
        n = len(words)
        cdef char **c_words = <char **> malloc(n * sizeof(char *))
        for i in range(n):
            c_words[i] = <char *> malloc(len(words[i]) * sizeof(char))
            strcpy(c_words[i], words[i].encode())
        word = Word.create(<object> ctrie.trie_get_nwp(self._c_trie, <const char **> c_words, n))
        for i in range(n):
            free(<void *> c_words[i])
        free(<void *> c_words)
        return word


def build(order: int, arpa_path: str, out_path: str):
    def job():
        cdef ctrie.trie *t = ctrie.trie_new_from_arpa_path(order, arpa_path.encode())
        cdef FILE *f = fopen(out_path.encode(), "wb".encode());
        if f is NULL:
            raise IOError("File could not be opened")
        ctrie.trie_fwrite(t, f)
        fclose(f)

    process = multiprocessing.Process(target=job)
    process.start()
    try:
        process.join()
    except KeyboardInterrupt:
        process.terminate()
        raise KeyboardInterrupt