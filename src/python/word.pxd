from libc.stdint cimport uint32_t


cdef extern from "word.h":
    ctypedef uint32_t word_id_type
    cdef struct word:
        pass

    const char *word_get_text(const word *word)