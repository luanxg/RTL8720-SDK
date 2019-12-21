SET(TEST_DIR ${CMAKE_CURRENT_LIST_DIR}/../../../..)
SET(TEST_NAME lightduer_dcs_speaker_control_test)

SET(TEST_FILE
    ${TEST_DIR}/testing/common_mock_functions.c
    ${TEST_DIR}/testing/baidu_json_mock_functions.c
    ${TEST_DIR}/testing/mutex_mock_functions.c
    ${TEST_DIR}/modules/dcs/lightduer_dcs_speaker_control.c
    ${TEST_DIR}/framework/core/lightduer_debug.c
    ${CMAKE_CURRENT_LIST_DIR}/../lightduer_dcs_common_mock.c
    ${CMAKE_CURRENT_LIST_DIR}/../lightduer_dcs_dummy_mock.c
    ${CMAKE_CURRENT_LIST_DIR}/lightduer_dcs_speaker_control_test.c
   )

SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,-T${TEST_DIR}/testing/unit_test.lds")
ADD_EXECUTABLE(${TEST_NAME} ${TEST_FILE} ${TEST_DIR}/testing/main.c)
TARGET_LINK_LIBRARIES(${TEST_NAME} cmocka)

SET(TEST_CASES
    ${TEST_CASES}
    "${CMAKE_CURRENT_BINARY_DIR}/${TEST_NAME}"
    CACHE INTERNAL
    "test cases"
    )
