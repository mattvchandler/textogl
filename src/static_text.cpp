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
#include "font_impl.hpp"

#include <vector>

#include <GL/glew.h>

namespace textogl
{
    /// Implementation details for font and text rendering
    struct Static_text::Impl
    {
        /// Create and build text object
        /// @param font Font_sys object containing desired font. A pointer
        ///        to this is stored internally, so the Font_sys object must
        ///        remain valid for the life of the Static_text object
        /// @param utf8_input Text to render, in UTF-8 encoding. For best performance, normalize the string before rendering
        Impl(Font_sys & font,
                    const std::string & utf8_input
                   );
        ~Impl();

        /// @name Non-copyable
        /// @{
        Impl(const Impl &) = delete;
        Impl & operator=(const Impl &) = delete;
        /// @}

        /// @name Movable
        /// @{
        Impl(Impl && other);
        Impl & operator=(Impl && other);
        /// @}

        /// Recreate text object with new Font_sys

        /// Useful when Font_sys::resize has been called

        /// @param font Font_sys object containing desired font. A pointer
        ///        to this is stored internally, so the Font_sys object must
        ///        remain valid for the life of the Static_text object
        void set_font_sys(Font_sys & font);

        /// Recreate text object with new string

        /// @param utf8_input Text to render, in UTF-8 encoding. For best performance, normalize the string before rendering
        void set_text(const std::string & utf8_input);

        /// Render the previously set text
        void render_text(const Color & color,          ///< Text Color
                         const Vec2<float> & win_size, ///< Window dimensions. A Vec2 with X = width and Y = height
                         const Vec2<float> & pos,      ///< Render position, in screen pixels
                         const int align_flags = 0     ///< Text Alignment. Should be #Text_origin flags bitwise-OR'd together
                        );

        void rebuild(); ///< Rebuild text data

        /// Points to Font_sys chosen at construction.

        /// This object must remain valid for the whole lifetime of this Static_text object
        Font_sys::Impl * _font;

        std::string _text; ///< Text to render, in UTF-8 encoding.

        GLuint _vao; ///< OpenGL Vertex array object index
        GLuint _vbo; ///< OpenGL Vertex buffer object index

        std::vector<Font_sys::Impl::Coord_data> _coord_data; ///< Start and end indexs into \ref _vbo
        Font_sys::Impl::Bbox<float> _text_box;               ///< Bounding box for the text
    };

    Static_text::Static_text(Font_sys & font, const std::string & utf8_input): pimpl(new Impl(font, utf8_input), [](Impl * impl){ delete impl; }) {}
    Static_text::Impl::Impl(Font_sys & font, const std::string & utf8_input): _font(font.pimpl.get()),  _text(utf8_input)
    {
        glGenVertexArrays(1, &_vao);
        glBindVertexArray(_vao);
        glGenBuffers(1, &_vbo);
        glBindBuffer(GL_ARRAY_BUFFER, _vbo);

        // set up buffer obj properties, load vertex data
        rebuild();

        glBindVertexArray(_vao);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(Vec2<float>), NULL);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(Vec2<float>), (const GLvoid *)sizeof(Vec2<float>));
        glEnableVertexAttribArray(1);

        glBindVertexArray(0);
    }

    Static_text::Impl::~Impl()
    {
        // destroy VAO/VBO
        glDeleteBuffers(1, &_vbo);
        glDeleteVertexArrays(1, &_vao);
    }

    Static_text::Impl::Impl(Impl && other):
        _font(other._font),
        _text(other._text),
        _vao(other._vao),
        _vbo(other._vbo),
        _coord_data(std::move(other._coord_data)),
        _text_box(std::move(other._text_box))
    {
        other._vao = other._vbo = 0;
    }
    Static_text::Impl & Static_text::Impl::operator=(Impl && other)
    {
        if(this != &other)
        {
            _font = other._font;
            _text = other._text;
            _vao = other._vao;
            _vbo = other._vbo;
            _coord_data = std::move(other._coord_data);
            _text_box = std::move(other._text_box);

            other._vao = other._vbo = 0;
        }
        return *this;
    }

    void Static_text::set_font_sys(Font_sys & font)
    {
        pimpl->set_font_sys(font);
    }
    void Static_text::Impl::set_font_sys(Font_sys & font)
    {
        _font = font.pimpl.get();
        rebuild();
    }

    void Static_text::set_text(const std::string & utf8_input)
    {
        pimpl->set_text(utf8_input);
    }
    void Static_text::Impl::set_text(const std::string & utf8_input)
    {
        _text = utf8_input;
        rebuild();
    }

    void Static_text::render_text(const Color & color, const Vec2<float> & win_size,
            const Vec2<float> & pos, const int align_flags)
    {
        pimpl->render_text(color, win_size, pos, align_flags);
    }
    void Static_text::Impl::render_text(const Color & color, const Vec2<float> & win_size,
            const Vec2<float> & pos, const int align_flags)
    {
        _font->render_text_common(color, win_size, pos, align_flags, _text_box, _coord_data, _vao);
    }

    void Static_text::Impl::rebuild()
    {
        // build the text
        std::vector<Vec2<float>> coords;
        std::tie(coords, _coord_data, _text_box) = _font->build_text(_text);

        glBindVertexArray(_vao);
        glBindBuffer(GL_ARRAY_BUFFER, _vbo);

        // reload vertex data
        glBufferData(GL_ARRAY_BUFFER, sizeof(Vec2<float>) * coords.size(), coords.data(), GL_STATIC_DRAW);

        glBindVertexArray(0);
    }

}
