cmake_minimum_required (VERSION 3.5)

#Where to find local cmake scripts
set(OS_CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/../cmake/modules)
message(OS_CMAKE_MODULE_PATH="${OS_CMAKE_MODULE_PATH}")
set(CMAKE_MODULE_PATH ${OS_CMAKE_MODULE_PATH})

project(openspy)
enable_testing()

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

include_directories("core")


set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib/")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib/")

include_directories (${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/core/SSL/SSLNull)

if(UNIX)
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -std=gnu++0x -fpermissive")
endif()

add_subdirectory(core)
add_subdirectory(utmaster)
add_subdirectory(GP)
add_subdirectory(gamestats)
add_subdirectory(search)
add_subdirectory(FESL)
add_subdirectory(serverbrowsing)
add_subdirectory(qr)
add_subdirectory(natneg)
add_subdirectory(peerchat)
add_subdirectory(cdkey)