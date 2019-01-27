ENABLE_TESTING()

IF (MSVC)
    SET(AUX ../deps/xxhash/xxhash.c ../wincompat/getopt.c ../wincompat/getopt1.c)
ELSE()
    SET(AUX ../deps/xxhash/xxhash.c)
ENDIF()

FOREACH(TEST_NAME int enc_str huff_dec read_enc_stream qpack)
    ADD_EXECUTABLE(test_${TEST_NAME} test_${TEST_NAME}.c ${AUX})
    TARGET_LINK_LIBRARIES(test_${TEST_NAME} ls-qpack)
    ADD_TEST(${TEST_NAME} test_${TEST_NAME})
ENDFOREACH()

FILE(GLOB SCENARIOS scenarios/*.sce)
FOREACH(SCENARIO ${SCENARIOS})
    GET_FILENAME_COMPONENT(TEST_NAME ${SCENARIO} NAME)
    ADD_TEST(NAME scenario-${TEST_NAME}
             COMMAND bash run-scenario.sh ${SCENARIO})
    SET_TESTS_PROPERTIES(scenario-${TEST_NAME} PROPERTIES
	    ENVIRONMENT "PATH=$ENV{PATH}:../bin:../Debug/bin:../tools")
ENDFOREACH()

FILE(GLOB QIFS qifs/*.qif)
FOREACH(QIF ${QIFS})
    GET_FILENAME_COMPONENT(FILE_NAME ${QIF} NAME)
    FOREACH(TABLE_SIZE 0 256 512 1024 4096)
        FOREACH(RISKED_STREAMS 0 100)
            FOREACH(IMMEDIATE_ACK 0 1)
                FOREACH(AGGRESSIVE 0 1)
                    SET(TEST_NAME qif-${FILE_NAME}-t${TABLE_SIZE}-s${RISKED_STREAMS}-a${IMMEDIATE_ACK}-A${AGGRESSIVE})
                    ADD_TEST(NAME ${TEST_NAME}
                             COMMAND perl run-qif.pl --table-size ${TABLE_SIZE} --risked-streams ${RISKED_STREAMS} --immed-ack ${IMMEDIATE_ACK} --aggressive ${AGGRESSIVE} ${QIF})
                ENDFOREACH()
            ENDFOREACH()
        ENDFOREACH()
    ENDFOREACH()
ENDFOREACH()
