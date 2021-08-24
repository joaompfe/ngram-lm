cimport word as cword


cdef class Word:
    cdef cword.word *_c_word

    def __cinit__(self):
        pass

    @staticmethod
    def create(w):
        word = Word()
        word._c_word = <cword.word *> w
        return word

    def __str__(self):
        return cword.word_get_text(self._c_word).decode()