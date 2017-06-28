// font_libs.cpp
// shared font libraries

// Copyright 2015 Matthew Chandler

// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#include "textogl/font.hpp"

#include <system_error>

// GLSL shaders
// TODO: include these from separate file (so we can have syntax highlighting, etc
const GLchar * const vert_shader_src = R"(
#version 130

in vec2 vert_pos;
in vec2 vert_tex_coords;

uniform vec2 start_offset;
uniform vec2 win_size;

out vec2 tex_coord;

void main()
{
    tex_coord = vert_tex_coords;
    // convert pixel coords to screen coords
    vec2 pix_pos = vert_pos + start_offset;
    gl_Position = vec4(pix_pos.x * 2.0 / win_size.x - 1.0,
        1.0 - pix_pos.y * 2.0 / win_size.y, 0.0, 1.0);
}
)";

const GLchar * const frag_shader_src = R"(
#version 130

in vec2 tex_coord;

uniform sampler2D font_page;
uniform vec4 color;

out vec4 frag_color;

void main()
{
    // get alpha from font texture
    frag_color = vec4(color.rgb, color.a * textureLod(font_page, tex_coord, 0.0).r);
}
)";

namespace textogl
{
    Font_sys::Font_common::Font_common()
    {
        FT_Error err = FT_Init_FreeType(&ft_lib);
        if(err != FT_Err_Ok)
        {
            throw std::system_error(err, std::system_category(), "Error loading freetype library");
        }

        GLuint vert = glCreateShader(GL_VERTEX_SHADER);
        GLuint frag = glCreateShader(GL_FRAGMENT_SHADER);

        glShaderSource(vert, 1, &vert_shader_src, NULL);
        glShaderSource(frag, 1, &frag_shader_src, NULL);

        for(auto i : {vert, frag})
        {
            glCompileShader(i);

            GLint compile_status;
            glGetShaderiv(i, GL_COMPILE_STATUS, &compile_status);

            if(compile_status != GL_TRUE)
            {
                GLint log_length;
                glGetShaderiv(i, GL_INFO_LOG_LENGTH, &log_length);
                std::vector<char> log(log_length + 1);
                log.back() = '\0';
                glGetShaderInfoLog(i, log_length, NULL, log.data());

                glDeleteShader(vert);
                glDeleteShader(frag);
                FT_Done_FreeType(ft_lib);

                // TODO: some information about which shader failed would be nice
                throw std::system_error(compile_status, std::system_category(), std::string("Error compiling shader: \n") +
                        std::string(log.data()));
            }
        }

        // create program and attatch new shaders to it
        prog = glCreateProgram();
        glAttachShader(prog, vert);
        glAttachShader(prog, frag);

        // set attr locations
        glBindAttribLocation(prog, 0, "vert_pos");
        glBindAttribLocation(prog, 1, "vert_tex_coords");

        glLinkProgram(prog);

        // detatch and delete shaders
        glDetachShader(prog, vert);
        glDetachShader(prog, frag);
        glDeleteShader(vert);
        glDeleteShader(frag);

        // check for link errors
        GLint link_status;
        glGetProgramiv(prog, GL_LINK_STATUS, &link_status);
        if(link_status != GL_TRUE)
        {
            GLint log_length;
            glGetProgramiv(prog, GL_INFO_LOG_LENGTH, &log_length);
            std::vector<char> log(log_length + 1);
            log.back() = '\0';
            glGetProgramInfoLog(prog, log_length, NULL, log.data());

            FT_Done_FreeType(ft_lib);
            glDeleteProgram(prog);

            throw std::system_error(link_status, std::system_category(), std::string("Error linking shader program:\n") +
                    std::string(log.data()));
        }

        // get uniform locations
        for(auto & uniform : {"start_offset", "win_size", "font_page", "color"})
        {
            GLint loc = glGetUniformLocation(prog, uniform);
            if(loc != -1)
            {
                uniform_locations[uniform] = loc;
            }
        }
    }

    Font_sys::Font_common::~Font_common()
    {
        FT_Done_FreeType(ft_lib);
        glDeleteProgram(prog);
    }

    unsigned int Font_sys::_common_ref_cnt = 0;
    std::unique_ptr<Font_sys::Font_common> Font_sys::_common_data;
}