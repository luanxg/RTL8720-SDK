PROJECT(LightDuerTest C)
CMAKE_MINIMUM_REQUIRED(VERSION 3.5)

#cmocka lib directory
LINK_DIRECTORIES(/usr/local/lib)

SET(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)
message(STATUS ${CMAKE_CXX_COMPILER_ID})
#code coverage
INCLUDE(CodeCoverage)
APPEND_COVERAGE_COMPILER_FLAGS()
SET(COVERAGE_EXCLUDES '${CMAKE_SOURCE_DIR}/*')

SET(TEST_BASE_DIR ${CMAKE_CURRENT_LIST_DIR}/..)
INCLUDE_DIRECTORIES(
    ${CMAKE_SOURCE_DIR}
    ${TEST_BASE_DIR}
    ${TEST_BASE_DIR}/framework/include/
    ${TEST_BASE_DIR}/framework/core/
    ${TEST_BASE_DIR}/framework/utils/
    ${TEST_BASE_DIR}/platform/include/
    ${TEST_BASE_DIR}/external/baidu_json
    ${TEST_BASE_DIR}/external/mbedtls/include
    ${TEST_BASE_DIR}/modules/connagent
    ${TEST_BASE_DIR}/modules/coap
    ${TEST_BASE_DIR}/modules/voice_engine
    ${TEST_BASE_DIR}/modules/ntp
    ${TEST_BASE_DIR}/modules/play_event
    ${TEST_BASE_DIR}/modules/dcs
)

SET(TEST_CASES "" CACHE INTERNAL "test cases")

ADD_SUBDIRECTORY(modules)
ADD_SUBDIRECTORY(framework)

MESSAGE("test--->: ${TEST_CASES}")

SETUP_TARGET_FOR_COVERAGE(
    NAME ut_test
    EXECUTABLE ${PROJECT_SOURCE_DIR}/testcases.sh ${TEST_CASES}
    DEPENDENCIES ${TEST_CASES}
)

