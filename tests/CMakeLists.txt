# This file (c) 2015 AlertAvert.com.  All rights reserved.

project(execmod_test)

enable_testing()

set(TEST_FILES
        ${TESTS_DIR}/test_cmdexecute.cpp
)

set(TEST_TARGET ${PROJECT_NAME})

find_package(Protobuf REQUIRED)
include_directories(${PROTOBUF_INCLUDE_DIRS})
PROTOBUF_GENERATE_CPP(PROTO_SRCS PROTO_HDRS ${PROTO_DIR}/execute.proto)

add_executable(${TEST_TARGET}
        ${SOURCE_FILES}
        ${PROTO_SRCS} ${PROTO_HDRS}
        ${TEST_FILES})
target_link_libraries(${TEST_TARGET}
        gtest gtest_main
        protobuf
        mesos)

add_test(${TEST_TARGET} ${CMAKE_CURRENT_BINARY_DIR}/${TEST_TARGET})
