set(SRCS
    active_list_window.cc
    active_list_window.h
    app.cc
    app.h
    binding.cc
    binding.h
    binding_config.cc
    binding_config.h
    book_frame.cc
    book_frame.h
    config.cc
    config.h
    font_util.cc
    font_util.h
    id.h
    i18n_strings.h
    i18n_util.cc
    i18n_util.h
    notebook.cc
    notebook.h
    option.cc
    option.h
    session.cc
    session.h
    skin.cc
    skin.h
    text_page.cc
    text_page.h
    )

set(TARGET_NAME app)

add_executable(${TARGET_NAME} WIN32 MACOSX_BUNDLE ${SRCS})
if(WIN32)
    target_link_libraries(${TARGET_NAME} gdiplus)
endif()
target_link_libraries(${TARGET_NAME} ${wxWidgets_LIBRARIES} base editor libconfig uchardet)
set_target_properties(${TARGET_NAME} PROPERTIES RUNTIME_OUTPUT_NAME jil)
if(MSVC)
    set_target_properties(${TARGET_NAME} PROPERTIES COMPILE_FLAGS "/wd4290")
endif()

# Unit test.
if(JIL_ENABLE_TEST)
    set(UT_SRCS
        font_util_unittest.cc
        #unittest_main.cc
        )
    set(UT_TARGET_NAME app_unittest)
    add_executable(${UT_TARGET_NAME} ${UT_SRCS})
    target_link_libraries(${UT_TARGET_NAME} ${wxWidgets_LIBRARIES} base editor libconfig ${GTEST_BOTH_LIBRARIES})
endif()

# Install.
# TODO: MAC
install(TARGETS ${TARGET_NAME} RUNTIME DESTINATION .)
