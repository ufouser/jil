set(HEADERS
    binding.h
    buffer_listener.h
    color.h
    compile_config.h
    defs.h
    edit_action.h
    file_io.h
    file_type.h
    graphics.h
    key.h
    lex.h
    line_nr_area.h
    notebook.h
    option.h
    renderer.h
    seeker.h
    status_line.h
    style.h
    text_area.h
    text_buffer.h
    text_extent.h
    text_line.h
    text_point.h
    text_range.h
    text_window.h
    tab.h
    theme.h
    tip.h
    wrap.h
    )

set(SRCS
    binding.cc
    color.cc
    edit_action.cc
    file_io.cc
    file_type.cc
    graphics.cc
    key.cc
    lex.cc
    line_nr_area.cc
    notebook.cc
    renderer.cc
    seeker.cc
    status_line.cc
    style.cc
    text_area.cc
    text_buffer.cc
    text_extent.cc
    text_line.cc
    text_range.cc
    text_window.cc
    tab.cc
    theme.cc
    tip.cc
    wrap.cc
    )

add_library(editor ${SRCS} ${HEADERS})

# Unit test.
if(JIL_ENABLE_TEST)
    set(UT_SRCS
        edit_action_unittest.cc
        text_line_unittest.cc
        text_buffer_unittest.cc
        text_range_unittest.cc
        tab_util_unittest.cc
        wrap_unittest.cc
        wrap_line_unittest.cc
        unittest_main.cc
        )
    set(UT_TARGET_NAME editor_unittest)
    add_executable(${UT_TARGET_NAME} ${UT_SRCS})
    target_link_libraries(${UT_TARGET_NAME} ${wxWidgets_LIBRARIES} base editor ${GTEST_LIBRARIES} uchardet)

    add_test(${UT_TARGET_NAME} ${UT_TARGET_NAME})
endif()

# Performance test.
if(JIL_ENABLE_PERFORMANCE)
    set(PERFORMANCE_SRCS
        performance_util.cc
        performance_util.h
        text_line_performance.cc
        )
    set(PERFORMANCE_TARGET_NAME editor_performance)
    add_executable(${PERFORMANCE_TARGET_NAME} ${PERFORMANCE_SRCS})
    target_link_libraries(${PERFORMANCE_TARGET_NAME} ${wxWidgets_LIBRARIES} base editor)
endif()
