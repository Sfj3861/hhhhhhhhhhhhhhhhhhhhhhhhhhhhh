cmake_minimum_required(VERSION 3.20)

project(RetroPort VERSION 0.1.0 LANGUAGES CXX)

option(RETROPORT_BUILD_TESTS "Build RetroPort tests" ON)

add_library(retroport_core
    src/rom.cpp
)

target_include_directories(retroport_core
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

target_compile_features(retroport_core PUBLIC cxx_std_20)

if(MSVC)
    target_compile_options(retroport_core PRIVATE /W4 /permissive-)
else()
    target_compile_options(retroport_core PRIVATE -Wall -Wextra -Wpedantic -Wconversion)
endif()

add_executable(retroport
    src/main.cpp
)

target_link_libraries(retroport PRIVATE retroport_core)

if(RETROPORT_BUILD_TESTS)
    include(CTest)

    if(BUILD_TESTING)
        add_executable(retroport_tests
            tests/rom_tests.cpp
        )
        target_link_libraries(retroport_tests PRIVATE retroport_core)
        add_test(NAME retroport_tests COMMAND retroport_tests)
    endif()
endif()
