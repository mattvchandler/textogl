find_package(OpenGL REQUIRED)
find_package(GLEW REQUIRED)
find_package(Freetype REQUIRED)

include_directories(
    ${CMAKE_CURRENT_LIST_DIR}/../include
    ${PROJECT_BINARY_DIR}
    ${FREETYPE_INCLUDE_DIRS}
    ${OPENGL_INCLUDE_DIRS}
    ${GLEW_INCLUDE_DIRS}
    )

add_library(textogl
    font.cpp
    font_common.cpp
    static_text.cpp
    )

target_link_libraries(textogl
    ${FREETYPE_LIBRARIES}
    ${OPENGL_LIBRARIES}
    ${GLEW_LIBRARIES}
    )

# load the shader source code into C++ strings
file(GENERATE OUTPUT ${PROJECT_BINARY_DIR}/shaders.cmake CONTENT "
    file(READ ${CMAKE_CURRENT_LIST_DIR}/shaders/font.vert VERT_SHADER)
    file(READ ${CMAKE_CURRENT_LIST_DIR}/shaders/font.frag FRAG_SHADER)
    configure_file(${CMAKE_CURRENT_LIST_DIR}/shaders/shaders.cpp.in
        ${PROJECT_BINARY_DIR}/shaders.cpp)
   ")

add_custom_target(shaders
    COMMAND ${CMAKE_COMMAND} -P ${PROJECT_BINARY_DIR}/shaders.cmake
    DEPENDS
        ${CMAKE_CURRENT_LIST_DIR}/shaders/shaders.cpp.in
        ${CMAKE_CURRENT_LIST_DIR}/shaders/font.vert
        ${CMAKE_CURRENT_LIST_DIR}/shaders/font.frag
    BYPRODUCTS
        ${PROJECT_BINARY_DIR}/shaders.cpp
    COMMENT "Including shader source files"
    )

add_dependencies(textogl shaders)
