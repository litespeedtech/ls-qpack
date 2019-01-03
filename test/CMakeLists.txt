ENABLE_TESTING()

FOREACH(TEST_NAME int enc_str huff_dec read_enc_stream qpack)
    ADD_EXECUTABLE(test_${TEST_NAME} test_${TEST_NAME}.c ../deps/xxhash/xxhash.c)
    TARGET_LINK_LIBRARIES(test_${TEST_NAME} ls-qpack)
    ADD_TEST(${TEST_NAME} test_${TEST_NAME})
ENDFOREACH()

FILE(GLOB SCENARIOS scenarios/*.sce)
FOREACH(SCENARIO ${SCENARIOS})
    GET_FILENAME_COMPONENT(TEST_NAME ${SCENARIO} NAME)
    ADD_TEST(NAME scenario-${TEST_NAME}
             COMMAND bash run-scenario.sh ${SCENARIO})
    SET_TESTS_PROPERTIES(scenario-${TEST_NAME} PROPERTIES
                            ENVIRONMENT "PATH=$ENV{PATH}:../bin:../tools")
ENDFOREACH()
