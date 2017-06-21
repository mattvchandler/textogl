cmake_minimum_required(VERSION 3.0)

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
