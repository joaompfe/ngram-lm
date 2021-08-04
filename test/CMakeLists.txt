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
        array_test.cc bit_test.cc)
target_link_libraries(
        ngram_lm_test
        ngram_lm
        gtest_main
)

include(GoogleTest)
gtest_discover_tests(ngram_lm_test)