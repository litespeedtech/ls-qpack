ENABLE_TESTING()

IF (MSVC)
    SET(AUX ../deps/xxhash/xxhash.c ../wincompat/getopt.c ../wincompat/getopt1.c)
ELSE()
    SET(AUX ../deps/xxhash/xxhash.c)
ENDIF()

FOREACH(BIN_NAME interop-encode interop-decode encode-int fuzz-decode)
    ADD_EXECUTABLE(${BIN_NAME} ${BIN_NAME}.c ${AUX})
    TARGET_LINK_LIBRARIES(${BIN_NAME} ls-qpack-noxxh)
ENDFOREACH()
