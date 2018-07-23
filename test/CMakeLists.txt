ENABLE_TESTING()

FOREACH(TEST_NAME qpack enc_str)
    ADD_EXECUTABLE(test_${TEST_NAME} test_${TEST_NAME}.c ../deps/xxhash/xxhash.c)
    TARGET_LINK_LIBRARIES(test_${TEST_NAME} ls-qpack)
    ADD_TEST(${TEST_NAME} test_${TEST_NAME})
ENDFOREACH()
