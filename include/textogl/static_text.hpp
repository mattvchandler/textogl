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

#ifndef STATIC_TEXT_HPP
#define STATIC_TEXT_HPP

#include "font.hpp"

/// @ingroup textogl
namespace textogl
{
    /// object for text which does not change often
    class Static_text final
    {
        public:
            // create and build text buffer object
            Static_text(Font_sys & font, const std::string & utf8_input, const textogl::Color & color);
            ~Static_text();
            // recreate text object with new string
            void set_text(Font_sys & font, const std::string & utf8_input);
            // set font color
            void set_color(const textogl::Color & color);
            // render the text
            void render_text(Font_sys & font, const textogl::Vec2<float> & win_size,
                    const textogl::Vec2<float> & pos, const int align_flags = 0);

        protected:
            GLuint _vao;
            GLuint _vbo;
            textogl::Color _color;
            std::vector<Font_sys::Coord_data> _coord_data;
            Font_sys::Bbox<float> _text_box;
    };
}

#endif // STATIC_TEXT_HPP
