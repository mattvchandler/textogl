// static_text.cpp
// static text object

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

// TODO: cleaunp unneeded textogls and add doxygen comments

// create and build text buffer object
textogl::Static_text::Static_text(Font_sys & font, const std::string & utf8_input):
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

textogl::Static_text::~Static_text()
{
    // destroy VAO/VBO
    glDeleteBuffers(1, &_vbo);
    glDeleteVertexArrays(1, &_vao);
}

textogl::Static_text::Static_text(Static_text && other):
    _font(other._font),
    _vao(other._vao),
    _vbo(other._vbo),
    _coord_data(std::move(other._coord_data)),
    _text_box(std::move(other._text_box))
{
    other._vao = other._vbo = 0;
}
textogl::Static_text & textogl::Static_text::operator=(Static_text && other)
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

// recreate text object with new string
void textogl::Static_text::set_text(const std::string & utf8_input)
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

// render the text
void textogl::Static_text::render_text(const Color & color, const Vec2<float> & win_size,
    const Vec2<float> & pos, const int align_flags)
{
    Vec2<float> start_offset = pos;

    int horiz_align = align_flags & 0x3;

    // offset origin to align to text bounding box
    switch(horiz_align)
    {
    case Font_sys::ORIGIN_HORIZ_BASELINE:
        break;
    case Font_sys::ORIGIN_HORIZ_LEFT:
        start_offset.x -= _text_box.ul.x;
        break;
    case Font_sys::ORIGIN_HORIZ_RIGHT:
        start_offset.x -= _text_box.lr.x;
        break;
    case Font_sys::ORIGIN_HORIZ_CENTER:
        start_offset.x -= _text_box.ul.x + _text_box.width() / 2.0f;
        break;
    }

    int vert_align = align_flags & 0xC;
    switch(vert_align)
    {
    case Font_sys::ORIGIN_VERT_BASELINE:
        break;
    case Font_sys::ORIGIN_VERT_TOP:
        start_offset.y -= _text_box.ul.y;
        break;
    case Font_sys::ORIGIN_VERT_BOTTOM:
        start_offset.y -= _text_box.lr.y;
        break;
    case Font_sys::ORIGIN_VERT_CENTER:
        start_offset.y -= _text_box.lr.y + _text_box.height() / 2.0f;
        break;
    }

    // set up shader uniforms
    glUseProgram(_font->_common_data->prog);
    glUniform2fv(_font->_common_data->uniform_locations["start_offset"], 1, &start_offset[0]);
    glUniform2fv(_font->_common_data->uniform_locations["win_size"], 1, &win_size[0]);
    glUniform4fv(_font->_common_data->uniform_locations["color"], 1, &color[0]);

    glBindVertexArray(_vao);

    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glActiveTexture(GL_TEXTURE14);

    // draw text, per page
    for(const auto & cd: _coord_data)
    {
        // bind the page's texture
        glBindTexture(GL_TEXTURE_2D, _font->_page_map[cd.page_no].tex);
        glDrawArrays(GL_TRIANGLES, cd.start, cd.num_elements);
    }

    glBindVertexArray(0);
}
