# This file is (c) 2015 AlertAvert.com.  All rights reserved.

cmake_minimum_required(VERSION 3.2)
project(execmod)

# Version number
set(VERSION_MAJOR 0)
set(VERSION_MINOR 1)

# TODO: how do I use git SHA short commit instead?
string(TIMESTAMP BUILD_ID "%Y%m%d" UTC)


# Mesos installation directory
if (NOT MESOS_INSTALL_DIR)
    set(MESOS_INSTALL_DIR $ENV{MESOS_INSTALL})
endif(NOT MESOS_INSTALL_DIR)

if(NOT MESOS_INSTALL_DIR)
    message(WARNING "Mesos libraries/includes directory not configured,
                     using default (/usr/local): this may not be what you
                     wanted")
    set(MESOS_INSTALL_DIR /usr/local)
endif(NOT MESOS_INSTALL_DIR)


# Directories configuration
set(SOURCE_DIR ${PROJECT_SOURCE_DIR}/src)
set(PROJECT_BINARY_DIR ${PROJECT_SOURCE_DIR}/build)
set(GEN_DIR ${PROJECT_SOURCE_DIR}/gen)
set(CMAKE_PREFIX_PATH /usr/local)
set(PROTO_DIR ${PROJECT_SOURCE_DIR}/proto)

# The name of the Module
set(MODULE_NAME com_alertavert_mesos_RemoteExecution)

# Configure a header file to pass settings into source code
configure_file(
    ${PROJECT_SOURCE_DIR}/config.h.in
    ${GEN_DIR}/config.h
)

include_directories(${SOURCE_DIR}/include
                    ${GEN_DIR}
                    ${PROJECT_BINARY_DIR}/include
                    ${MESOS_INSTALL_DIR}/include
                    /usr/local/include
)
link_directories(${MESOS_INSTALL_DIR}/lib /usr/local/lib)


set(CMAKE_CXX_FLAGS
    "${CMAKE_CXX_FLAGS} -DPICOJSON_USE_INT64 -D__STDC_FORMAT_MACROS -std=c++11")

# Build protocol buffers
add_custom_command(
    OUTPUT ${GEN_DIR}/execute.pb.cc ${GEN_DIR}/execute.pb.h
    COMMAND /Users/marco/usr_local/bin/protoc -I=${PROTO_DIR} --cpp_out=${GEN_DIR} ${PROTO_DIR}/execute.proto
    DEPENDS ${PROTO_DIR}/execute.proto
)

set(SOURCE_FILES ${SOURCE_DIR}/execute_module.cpp
        ${GEN_DIR}/execute.pb.cc
        src/include/cmdexecute.hpp src/cmdexecute.cpp)

add_library(${PROJECT_NAME} MODULE ${SOURCE_FILES})
target_link_libraries(${PROJECT_NAME} mesos gtest)

# The library name and extension.
# NOTE: if you change the name/type of the library, update the following
# definitions too, as they drive the JSON modules descriptor file too.
set(LIB_NAME lib${PROJECT_NAME})
set(LIB_EXT so)

message("Building module `${PROJECT_NAME}`
         from: ${PROJECT_SOURCE_DIR}
         to: ${PROJECT_BINARY_DIR}
         created: ${LIB_NAME}.${LIB_EXT}")

# Configure the JSON module library descriptor.
# This will be used with --module to load the module.
configure_file(
    ${PROJECT_SOURCE_DIR}/modules.json.in
    ${GEN_DIR}/modules.json
)
