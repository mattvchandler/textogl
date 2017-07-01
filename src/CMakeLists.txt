find_package(OpenGL REQUIRED)
find_package(GLEW REQUIRED)
find_package(Freetype REQUIRED)

include_directories(
    ${CMAKE_CURRENT_LIST_DIR}/../include
    ${FREETYPE_INCLUDE_DIRS}
    ${OPENGL_INCLUDE_DIRS}
    ${GLEW_INCLUDE_DIRS}
    )

add_library(textogl
    font.cpp
    static_text.cpp
    font_common.cpp)

target_link_libraries(textogl
    ${FREETYPE_LIBRARIES}
    ${OPENGL_LIBRARIES}
    ${GLEW_LIBRARIES}
    )
