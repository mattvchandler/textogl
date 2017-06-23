cmake_minimum_required(VERSION 3.0)
project(textogl)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release")
endif()

set(CMAKE_CXX_FLAGS "-Wall")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")
set(CMAKE_CXX_FLAGS_DEBUG "-g -DDEBUG")

if(MSYS AND CMAKE_BUILD_TYPE STREQUAL "Release")
    set(CMAKE_EXE_LINKER_FLAGS "-mwindows")
endif()

# libraries
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/CMakeModules)

find_package(OpenGL REQUIRED)
find_package(SFML 2 COMPONENTS window system REQUIRED)

include_directories(
    ${OPENGL_INCLUDE_DIRS}
    ${SFML_INCLUDE_DIRS})

add_executable(textogl_demo
    demo/demo.cpp)

target_link_libraries(textogl_demo
    ${OPENGL_LIBRARIES}
    ${SFML_LIBRARIES})
