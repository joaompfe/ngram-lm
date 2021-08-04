add_library(ngram_lm STATIC ngram_trie.c ngram_trie.h array.c array.h bit.c bit.h)

include(FetchContent)
FetchContent_Declare(
        qlibc
        URL https://github.com/wolkykim/qlibc/archive/refs/heads/master.zip
)
FetchContent_MakeAvailable(qlibc)

target_link_libraries(ngram_lm PRIVATE qlibc-static)