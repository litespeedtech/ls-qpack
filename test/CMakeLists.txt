function(lsqpack_add_test TARGET)
    add_executable(test_${TARGET} "")
    target_sources(test_${TARGET} PRIVATE test_${TARGET}.c)
    target_link_libraries(test_${TARGET} ls-qpack)

    if(MSVC)
        target_include_directories(test_${TARGET} PRIVATE ../wincompat)
        target_sources(test_${TARGET} PRIVATE 
            ../wincompat/getopt.c 
            ../wincompat/getopt1.c
        )
    else()
        target_link_libraries(test_${TARGET} m)
    endif()

    add_test(${TARGET} test_${TARGET})
endfunction()

lsqpack_add_test(int)
lsqpack_add_test(enc_str)
lsqpack_add_test(huff_dec)
lsqpack_add_test(read_enc_stream)
lsqpack_add_test(qpack)
lsqpack_add_test(get_stx_id)

if(MSVC)
    message(WARNING "Scenario tests are disabled on Windows (TODO)")
else()
    file(GLOB SCENARIOS scenarios/*.sce)
    foreach(SCENARIO ${SCENARIOS})
        get_filename_component(TEST_NAME ${SCENARIO} NAME)
        add_test(
            NAME scenario-${TEST_NAME}
            COMMAND bash run-scenario.sh ${SCENARIO}
            WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/test
        )
        set_tests_properties(scenario-${TEST_NAME} PROPERTIES
            ENVIRONMENT "PATH=$ENV{PATH}:${PROJECT_BINARY_DIR}/bin:${PROJECT_SOURCE_DIR}/tools"
        )
    endforeach()
endif()

find_package(Perl)

if(PERL_FOUND)
    message(STATUS "Found Perl interpreter: ${PERL}")
    file(GLOB QIFS qifs/*.qif)
    
    foreach(QIF ${QIFS})
        get_filename_component(FILE_NAME ${QIF} NAME)
        
        foreach(TABLE_SIZE 0 256 512 1024 4096)
            foreach(RISKED_STREAMS 0 100)
                foreach(IMMEDIATE_ACK 0 1)
                    foreach(AGGRESSIVE 0 1)
                        set(TEST_NAME qif-${FILE_NAME}-t${TABLE_SIZE}-s${RISKED_STREAMS}-a${IMMEDIATE_ACK}-A${AGGRESSIVE})
                        
                        add_test(
                            NAME ${TEST_NAME}
                            COMMAND ${PERL} run-qif.pl --table-size ${TABLE_SIZE} --risked-streams ${RISKED_STREAMS} --immed-ack ${IMMEDIATE_ACK} --aggressive ${AGGRESSIVE} ${QIF}
                            WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/test
                        )
                        
                        if(NOT MSVC)
                            set_tests_properties(${TEST_NAME} PROPERTIES
                                ENVIRONMENT "PATH=$ENV{PATH}:${PROJECT_BINARY_DIR}/bin:${PROJECT_SOURCE_DIR}/tools"
                            )
                        endif()
                    endforeach()
                endforeach()
            endforeach()
        endforeach()
    endforeach()
else()
    message(WARNING "Perl not found: QIF tests won't be run")
endif()
