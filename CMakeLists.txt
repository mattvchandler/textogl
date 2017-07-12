cmake_minimum_required(VERSION 3.0)
project(textogl CXX)

set(TITLE "textogl")
set(AUTHOR "Matthew Chandler <tardarsauce@gmail.com>")
set(COPYRIGHT "2017 Matthew Chandler")
set(SUMMARY "OpenGL Text renderer")
set(WEBSITE "http://github.com/mattvchandler/textogl")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release")
endif()

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
add_compile_options(-Wall -Wextra)

set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")
set(CMAKE_CXX_FLAGS_DEBUG "-g -DDEBUG")

if(MSYS AND CMAKE_BUILD_TYPE STREQUAL "Release")
    set(CMAKE_EXE_LINKER_FLAGS "-mwindows")
endif()

# libraries
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/CMakeModules)

add_subdirectory(src)
add_subdirectory(demo)

# TODO versioning

# Doxygen documentation
option(TEXTOGL_GEN_DOCS "Generate 'doc' target" ON)
option(TEXTOGL_INTERNAL_DOCS "Generate documentation for private/internal methods, members, and functions" OFF)

if(TEXTOGL_GEN_DOCS)
    find_package(Doxygen)
    if(DOXYGEN_FOUND)

        configure_file(${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in
            ${PROJECT_BINARY_DIR}/Doxyfile)

        add_custom_command(
            OUTPUT ${PROJECT_BINARY_DIR}/doc/html/index.html
            COMMAND ${DOXYGEN_EXECUTABLE} ${PROJECT_BINARY_DIR}/Doxyfile
            WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
            DEPENDS ${PROJECT_BINARY_DIR}/Doxyfile
            # DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/README.md
            DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/include/textogl/*.hpp
            DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
            COMMENT "Generating documentation with Doxygen")

        add_custom_target(doc DEPENDS ${PROJECT_BINARY_DIR}/doc/html/index.html)

        set_directory_properties(PROPERTIES ADDITIONAL_MAKE_CLEAN_FILES doc)
    endif()
endif()
