# The following variable can be defined on the command line:
#
#   SHARED
#   NDEBUG
#   XXH_HEADER_NAME
#   XXH_INCLUDE_DIR

CMAKE_MINIMUM_REQUIRED(VERSION 2.8)
PROJECT(ls-qpack)

IF (SHARED EQUAL 1)
    ADD_LIBRARY(ls-qpack SHARED src/lsqpack.c deps/xxhash/xxhash.c)
    ADD_LIBRARY(ls-qpack-noxxh SHARED src/lsqpack.c)
ELSE()
    ADD_LIBRARY(ls-qpack STATIC src/lsqpack.c deps/xxhash/xxhash.c)
    ADD_LIBRARY(ls-qpack-noxxh STATIC src/lsqpack.c)
ENDIF()
INCLUDE_DIRECTORIES(include)

IF (NOT DEFINED XXH_HEADER_NAME)
    SET(XXH_HEADER_NAME "xxhash.h")
ENDIF()

IF (DEFINED XXH_INCLUDE_DIR)
    INCLUDE_DIRECTORIES("${XXH_INCLUDE_DIR}")
ELSE()
    INCLUDE_DIRECTORIES(deps/xxhash)
ENDIF()

IF (MSVC)
    INCLUDE_DIRECTORIES(wincompat)
ENDIF()

IF (MSVC)
    SET(MY_CMAKE_FLAGS "${MY_CMAKE_FLAGS} /Wall")
    SET(MY_CMAKE_FLAGS "${MY_CMAKE_FLAGS} /wd4100") # unreffed parameter
    SET(MY_CMAKE_FLAGS "${MY_CMAKE_FLAGS} /wd4200") # zero-sized array
    # Apparently this C99 construct is not supported properly by VS:
    #   https://stackoverflow.com/questions/1064930/struct-initializer-typedef-with-visual-studio
    SET(MY_CMAKE_FLAGS "${MY_CMAKE_FLAGS} /wd4204") # non-constant aggregate initializer
    SET(MY_CMAKE_FLAGS "${MY_CMAKE_FLAGS} /wd4255") # no function prototype (getopt)
    SET(MY_CMAKE_FLAGS "${MY_CMAKE_FLAGS} /wd4820") # padding
    SET(MY_CMAKE_FLAGS "${MY_CMAKE_FLAGS} /wd4668") # undefined macro
    SET(MY_CMAKE_FLAGS "${MY_CMAKE_FLAGS} /wd4710") # not inlined by default
    SET(MY_CMAKE_FLAGS "${MY_CMAKE_FLAGS} /wd4996") # unsafe function
ELSE()
    SET(MY_CMAKE_FLAGS "${MY_CMAKE_FLAGS} -Wall -Wextra -Wno-unused-parameter")
    SET(MY_CMAKE_FLAGS "${MY_CMAKE_FLAGS} -fno-omit-frame-pointer")
ENDIF()
SET(MY_CMAKE_FLAGS "${MY_CMAKE_FLAGS} $ENV{EXTRA_CFLAGS}")

IF (MSVC)
    SET(CMAKE_C_FLAGS_DEBUG "/Od /Zi")
    SET(CMAKE_C_FLAGS_RELEASE "/O2")
ELSE()
    SET(CMAKE_C_FLAGS_DEBUG "-O0 -g3")
    SET(CMAKE_C_FLAGS_RELEASE "-O3 -g0")
ENDIF()

IF (NDEBUG EQUAL 1)
    SET(MY_CMAKE_FLAGS "${MY_CMAKE_FLAGS} -DNDEBUG")
ENDIF()

SET(MY_CMAKE_FLAGS "${MY_CMAKE_FLAGS} -DXXH_HEADER_NAME=\\\"${XXH_HEADER_NAME}\\\"")

SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${MY_CMAKE_FLAGS}")
SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} $ENV{EXTRA_LDFLAGS}")

MESSAGE(STATUS "Compiler flags: ${CMAKE_C_FLAGS}")

IF (NOT NDEBUG EQUAL 1)
    ENABLE_TESTING()
    INCLUDE_DIRECTORIES("test")
    ADD_SUBDIRECTORY("test")
ENDIF()

ADD_SUBDIRECTORY("bin")
