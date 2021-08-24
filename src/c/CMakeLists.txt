option(ngram_lm_static_build "Make static build" ON)
option(ngram_lm_shared_build "Make shared build" ON)

add_library(ngram_lm_o OBJECT trie.c trie.h array.c array.h bit.c bit.h ngram.c ngram.h word.h arpa.c arpa.h util/log.c util/log.h util/progress.h util/murmur3.c util/murmur3.h)

if (${ngram_lm_static_build})
    target_link_libraries(ngram_lm_o PRIVATE m)
    add_library(ngram_lm_static STATIC $<TARGET_OBJECTS:ngram_lm_o>)
    target_compile_options(ngram_lm_static PRIVATE -pedantic -Wall -Wextra)
endif ()
if (${ngram_lm_shared_build})
    set_property(TARGET ngram_lm_o PROPERTY POSITION_INDEPENDENT_CODE 1)
    add_library(ngram_lm SHARED $<TARGET_OBJECTS:ngram_lm_o>)
    target_link_libraries(ngram_lm PRIVATE m)
    target_compile_options(ngram_lm PRIVATE -pedantic -Wall -Wextra)
endif ()

add_executable(build build.c)
target_link_libraries(build ngram_lm)
target_compile_options(build PRIVATE -pedantic -Wall -Wextra -Wno-missing-field-initializers)


### TEST ###
include(FetchContent)
FetchContent_Declare(
        googletest
        URL https://github.com/google/googletest/archive/refs/heads/master.zip
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

enable_testing()

include_directories(${PROJECT_SOURCE_DIR}/src)

add_executable(
        ngram_lm_test
        trie_test.cc
        array_test.cc bit_test.cc arpa_test.cc)
target_link_libraries(
        ngram_lm_test
        ngram_lm
        gtest_main
)

include(GoogleTest)
gtest_discover_tests(ngram_lm_test)
