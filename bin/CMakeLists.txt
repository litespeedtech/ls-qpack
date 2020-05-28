function(lsqpack_add_executable TARGET)
    add_executable(${TARGET} "")
    target_link_libraries(${TARGET} PRIVATE ls-qpack)

    target_sources(${TARGET} PRIVATE ${TARGET}.c ../deps/xxhash/xxhash.c)
    target_include_directories(${TARGET} PRIVATE ../deps/xxhash)

    if(MSVC)
        target_include_directories(${TARGET} PRIVATE ../wincompat)
        target_sources(${TARGET} PRIVATE 
            ../wincompat/getopt.c 
            ../wincompat/getopt1.c
        )
    else()
        target_link_libraries(${TARGET} PRIVATE m)
    endif()
endfunction()

lsqpack_add_executable(interop-encode)
lsqpack_add_executable(interop-decode)
lsqpack_add_executable(encode-int)
lsqpack_add_executable(fuzz-decode)

target_include_directories(interop-decode PRIVATE ../test)
