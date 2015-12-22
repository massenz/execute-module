# This file (c) 2015 AlertAvert.com.  All rights reserved.

cmake_minimum_required(VERSION 3.2)
project(execmod)

# Include common cmake utilities:
include(${COMMON_UTILS_DIR}/commons.cmake OPTIONAL RESULT_VARIABLE COMMONS_FILE)


### Setting up common configuration variables


# Version number
set(RELEASE_MAJOR 0)
set(RELEASE_MINOR 2)
set(RELEASE_PATCH 0)

IF (${COMMONS_FILE} STREQUAL NOTFOUND)
    message(WARNING "Not using CMake utilities")
    set(BUILD_ID 0)
ELSE (${COMMONS_FILE} STREQUAL NOTFOUND)
    get_build_id(BUILD_ID)
ENDIF (${COMMONS_FILE} STREQUAL NOTFOUND)

set(RELEASE_STRING "${RELEASE_MAJOR}.${RELEASE_MINOR}.${RELEASE_PATCH}-${BUILD_ID}")
message(STATUS "Building Release: ${RELEASE_STRING}")

# The name of the Module
set(MODULE_NAME com_alertavert_mesos_RemoteExecution)

# NOTE: if you change the name/type of the library, update the following
# definitions too, as they drive the JSON modules descriptor file too.
set(LIB_NAME lib${PROJECT_NAME})
set(LIB_EXT ${CMAKE_SHARED_LIBRARY_SUFFIX})


# Local installation directory
# Contains all necessary header and library files in its
# include/ and lib/ sub-dirs.
if (NOT LOCAL_INSTALL_DIR)
    if($ENV{LOCAL_INSTALL})
        set(LOCAL_INSTALL_DIR $ENV{LOCAL_INSTALL})
        message("Local libraries and includes directory configured to ${LOCAL_INSTALL_DIR}")
    else($ENV{LOCAL_INSTALL})
        message(WARNING "Local libraries and includes directory not configured.\n"
                "Use either the $LOCAL_INSTALL environment variable or define "
                "the LOCAL_INSTALL_DIR property (e.g., -DLOCAL_INSTALL_DIR=/usr/local).")
    endif($ENV{LOCAL_INSTALL})
endif(NOT LOCAL_INSTALL_DIR)


### Directories configuration


set(SOURCE_DIR ${PROJECT_SOURCE_DIR}/src)
set(TESTS_DIR ${PROJECT_SOURCE_DIR}/tests)

set(PROTO_DIR ${PROJECT_SOURCE_DIR}/proto)
set(GEN_DIR ${PROJECT_SOURCE_DIR}/gen)

set(PROJECT_BINARY_DIR ${PROJECT_SOURCE_DIR}/build)
set(CMAKE_PREFIX_PATH ${LOCAL_INSTALL_DIR} /usr/local)


### Generated files


# Configure a header file to pass settings into source code
configure_file(
    ${PROJECT_SOURCE_DIR}/config.h.in
    ${GEN_DIR}/config.h
)


# Configure the JSON module library descriptor.
# This will be used with --module to load the module.
configure_file(
    ${PROJECT_SOURCE_DIR}/modules.json.in
    ${GEN_DIR}/modules.json
)


### Configurations that will be shared between source/tests


include_directories(${SOURCE_DIR}/include
        ${GEN_DIR}
        ${CMAKE_BINARY_DIR}
        ${LOCAL_INSTALL_DIR}/include
)
link_directories(${LOCAL_INSTALL_DIR}/lib /usr/local/lib)


set(CMAKE_CXX_FLAGS
        "${CMAKE_CXX_FLAGS} -DPICOJSON_USE_INT64 -D__STDC_FORMAT_MACROS -std=c++11")


### Build Library and Tests from C++ sources:

find_package(Protobuf REQUIRED)
include_directories(${PROTOBUF_INCLUDE_DIRS})
PROTOBUF_GENERATE_CPP(PROTO_SRCS PROTO_HDRS ${PROTO_DIR}/execute.proto)

set(SOURCE_FILES
        ${SOURCE_DIR}/execute_module.cpp
        ${SOURCE_DIR}/cmdexecute.cpp
)

add_library(${PROJECT_NAME} MODULE
        ${SOURCE_FILES}
        ${PROTO_SRCS} ${PROTO_HDRS}
)
target_link_libraries(${PROJECT_NAME} mesos)

### Unit tests build is defined in the tests/ folder
add_subdirectory(${TESTS_DIR})

message(STATUS "Building module '${PROJECT_NAME}' from: ${PROJECT_SOURCE_DIR}")
