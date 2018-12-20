ENABLE_TESTING()

FOREACH(BIN_NAME interop-encode interop-decode encode-int fuzz-decode)
    ADD_EXECUTABLE(${BIN_NAME} ${BIN_NAME}.c ../deps/xxhash/xxhash.c)
    TARGET_LINK_LIBRARIES(${BIN_NAME} ls-qpack)
ENDFOREACH()
