SET(TEST_DIR ${CMAKE_CURRENT_LIST_DIR}/../../..)

SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,-T${TEST_DIR}/testing/unit_test.lds")

SET(TEST_NAME lightduer_queue_cache_test)
SET(TEST_FILE
        ${TEST_DIR}/framework/utils/lightduer_queue_cache.c
        ${CMAKE_CURRENT_LIST_DIR}/lightduer_queue_cache_test.c
   )

ADD_EXECUTABLE(${TEST_NAME} ${TEST_FILE} ${TEST_DIR}/testing/main.c)
TARGET_LINK_LIBRARIES(${TEST_NAME} cmocka)

SET(TEST_CASES
    ${TEST_CASES}
    "${CMAKE_CURRENT_BINARY_DIR}/${TEST_NAME}"
    CACHE INTERNAL
    "test cases"
    )

SET(TEST_NAME lightduer_net_util_test)
SET(TEST_FILE
        ${TEST_DIR}/framework/utils/lightduer_net_util.c
        ${CMAKE_CURRENT_LIST_DIR}/lightduer_net_util_test.c
   )

ADD_EXECUTABLE(${TEST_NAME} ${TEST_FILE} ${TEST_DIR}/testing/main.c)
TARGET_LINK_LIBRARIES(${TEST_NAME} cmocka)

SET(TEST_CASES
    ${TEST_CASES}
    "${CMAKE_CURRENT_BINARY_DIR}/${TEST_NAME}"
    CACHE INTERNAL
    "test cases"
    )

