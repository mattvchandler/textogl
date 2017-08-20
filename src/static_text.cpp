/// @file
/// @brief Static text object

// Copyright 2017 Matthew Chandler

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

#include "textogl/static_text.hpp"

// TODO: lot's of common functionality inbetween this and font.cpp. can we make more shared funcs?

namespace textogl
{
    Static_text::Static_text(Font_sys & font, const std::string & utf8_input):
        _font(&font)
    {
        // build the text
        auto coord_data = build_text(utf8_input, *_font, _text_box);

        _coord_data = coord_data.second;

        // set up buffer obj properties, load vertex data
        glGenVertexArrays(1, &_vao);
        glGenBuffers(1, &_vbo);

        glBindVertexArray(_vao);
        glBindBuffer(GL_ARRAY_BUFFER, _vbo);

        glBufferData(GL_ARRAY_BUFFER, sizeof(Vec2<float>) * coord_data.first.size(),
                coord_data.first.data(), GL_STATIC_DRAW);

        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(Vec2<float>), NULL);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(Vec2<float>), (const GLvoid *)sizeof(Vec2<float>));
        glEnableVertexAttribArray(1);

        glBindVertexArray(0);
    }

    Static_text::~Static_text()
    {
        // destroy VAO/VBO
        glDeleteBuffers(1, &_vbo);
        glDeleteVertexArrays(1, &_vao);
    }

    Static_text::Static_text(Static_text && other):
        _font(other._font),
        _vao(other._vao),
        _vbo(other._vbo),
        _coord_data(std::move(other._coord_data)),
        _text_box(std::move(other._text_box))
    {
        other._vao = other._vbo = 0;
    }
    Static_text & Static_text::operator=(Static_text && other)
    {
        if(this != &other)
        {
            _font = other._font;
            _vao = other._vao;
            _vbo = other._vbo;
            _coord_data = std::move(other._coord_data);
            _text_box = std::move(other._text_box);

            other._vao = other._vbo = 0;
        }
        return *this;
    }

    void Static_text::set_text(const std::string & utf8_input)
    {
        // build the text
        auto coord_data = build_text(utf8_input, *_font, _text_box);

        _coord_data = coord_data.second;

        glBindVertexArray(_vao);
        glBindBuffer(GL_ARRAY_BUFFER, _vbo);

        // reload vertex data
        glBufferData(GL_ARRAY_BUFFER, sizeof(Vec2<float>) * coord_data.first.size(),
                coord_data.first.data(), GL_STATIC_DRAW);

        glBindVertexArray(0);
    }

    void Static_text::render_text(const Color & color, const Vec2<float> & win_size,
            const Vec2<float> & pos, const int align_flags)
    {
        _font->render_text_common(color, win_size, pos, align_flags, _text_box, _coord_data, _vao);
    }
}
