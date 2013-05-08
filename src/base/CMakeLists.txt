set(SRCS
    compiler_specific.h
    foreach.h
    math_util.h
    string_util.cc
    string_util.h
    )

add_library(base ${SRCS})

# Unit test.
if(JIL_ENABLE_TEST)
    set(UT_SRCS
        string_util_unittest.cc
        )
    set(UT_TARGET_NAME base_unittest)
    add_executable(${UT_TARGET_NAME} ${UT_SRCS})
    target_link_libraries(${UT_TARGET_NAME} base ${GTEST_BOTH_LIBRARIES})
endif()
