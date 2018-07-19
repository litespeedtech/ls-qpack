ENABLE_TESTING()

ADD_EXECUTABLE(test_qpack test_qpack.c ../deps/xxhash/xxhash.c)
TARGET_LINK_LIBRARIES(test_qpack ls-qpack)
ADD_TEST(qpack test_qpack)
