# Root CMake configuration for the mt5bridge_cpp project
cmake_minimum_required(VERSION 3.15)

project(mt5bridge_cpp LANGUAGES CXX)

option(BUILD_EXAMPLES "Build example programs" ON)

find_package(Python3 REQUIRED COMPONENTS Development)
find_package(PkgConfig REQUIRED)
pkg_check_modules(JANSSON REQUIRED jansson)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

add_library(mt5_bridge SHARED src/mt5_bridge.cpp)
target_include_directories(mt5_bridge
    PUBLIC
        ${PROJECT_SOURCE_DIR}/include
        ${JANSSON_INCLUDE_DIRS}
)
target_link_libraries(mt5_bridge
    PUBLIC
        Python3::Python
        ${JANSSON_LIBRARIES}
)

if(BUILD_EXAMPLES)
    add_executable(usage_example examples/usage_example.cpp)
    target_link_libraries(usage_example PRIVATE ${JANSSON_LIBRARIES})
    set_target_properties(usage_example PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})

    add_executable(smoke_no_mt5 examples/smoke_no_mt5.cpp)
    target_link_libraries(smoke_no_mt5 PRIVATE ${JANSSON_LIBRARIES})
    set_target_properties(smoke_no_mt5 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
endif()

