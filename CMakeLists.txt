cmake_minimum_required(VERSION 2.8)
project(jil)

option(JIL_ENABLE_TEST "enable unit test?" OFF)
option(JIL_ENABLE_PERFORMANCE "enable performance test?" OFF)

# Enable unicode.
add_definitions(-DUNICODE -D_UNICODE)

# Find wxWidgets.
# Require "qa" for debug report.
find_package(wxWidgets 2.9.4 REQUIRED core base qa xml)
include(${wxWidgets_USE_FILE})

# Find boost.
# The latest CMake FindBoost script doesn't support boost 1.49.0.
set(Boost_ADDITIONAL_VERSIONS "1.49", "1.49.0")
find_package(Boost 1.47)
if(Boost_FOUND)
    include_directories(${Boost_INCLUDE_DIRS})
    link_directories(${Boost_LIBRARY_DIRS})
endif()

# Find gtest.
if(JIL_ENABLE_TEST)
    enable_testing()
    if (MSVC)
      set(GTEST_MSVC_SEARCH MD)
    endif()
    find_package(GTest REQUIRED)
    include_directories(${GTEST_INCLUDE_DIRS})
endif()

# Include the source directory.
include_directories(${PROJECT_SOURCE_DIR}/src)
# Include the third-party directory.
include_directories(${PROJECT_SOURCE_DIR}/third_party/include)

# Disable CRT secure warnings.
if(MSVC)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
endif()

add_subdirectory(src)

# Install Jil files (config, style, file type plugins, etc).
install(FILES ${PROJECT_SOURCE_DIR}/data/jilfiles/options.cfg DESTINATION .)
install(FILES ${PROJECT_SOURCE_DIR}/data/jilfiles/status_line.cfg DESTINATION .)
install(FILES ${PROJECT_SOURCE_DIR}/data/jilfiles/binding.cfg DESTINATION .)
install(DIRECTORY ${PROJECT_SOURCE_DIR}/data/jilfiles/theme DESTINATION .)
install(DIRECTORY ${PROJECT_SOURCE_DIR}/data/jilfiles/stub DESTINATION .)
