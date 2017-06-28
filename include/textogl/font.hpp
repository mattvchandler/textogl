/// @file
/// @brief Font loading and text display

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

#ifndef FONT_HPP
#define FONT_HPP

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include <cstdint>

#include <GL/glew.h>

#include <ft2build.h>
#include FT_FREETYPE_H

/// @ingroup textogl
namespace textogl
{
    struct Color
    {
        float r, g, b, a;
        float & operator[](std::size_t i) { return (&r)[i]; }
        const float & operator[](std::size_t i) const { return (&r)[i]; }
    };

    template<typename T>
    struct Vec2
    {
        T x, y;
        float & operator[](std::size_t i) { return (&x)[i]; }
        const float & operator[](std::size_t i) const { return (&x)[i]; }
    };

    /// Container for font and text rendering
    class Font_sys final
    {
        public:
            enum {ORIGIN_HORIZ_BASELINE = 0x00, ORIGIN_HORIZ_LEFT = 0x01, ORIGIN_HORIZ_RIGHT = 0x02, ORIGIN_HORIZ_CENTER = 0x03,
                ORIGIN_VERT_BASELINE = 0x00, ORIGIN_VERT_TOP = 0x04, ORIGIN_VERT_BOTTOM = 0x08, ORIGIN_VERT_CENTER = 0x0C};

            // Load font libraries and open a font file
            Font_sys(const std::string & font_path, const unsigned int font_size,
                    const unsigned int v_dpi = 96, const unsigned int h_dpi = 96);
            // deallocate font
            ~Font_sys();

            // render text (rebuilds for each frame - use Static_text if text doesn't change)
            void render_text(const std::string & utf8_input, const Color & color,
                    const Vec2<float> & win_size, const Vec2<float> & pos, const int align_flags = 0);

        protected:
            /// container for common libraries and shader program
            /// every Font_sys obj can use the same instance of these
            // TODO: copy/move ctors?
            class Font_common
            {
            public:
                Font_common();
                ~Font_common();

                FT_Library ft_lib;
                GLuint prog;
                std::unordered_map<std::string, GLuint> uniform_locations;
            };

            // common libraries
            static unsigned int _common_ref_cnt;
            static std::unique_ptr<Font_common> _common_data;

            // bounding box
            template<typename T>
            struct Bbox
            {
                Vec2<T> ul;
                Vec2<T> lr;

                T width() const
                {
                    return lr.x - ul.x;
                }
                T height() const
                {
                    return ul.y - lr.y;
                }
            };

            // vertex buffer coordinate data
            struct Coord_data
            {
                uint32_t page_no;
                std::size_t start;
                std::size_t num_elements;
            };

            // character info
            struct Char_info
            {
                Vec2<int> origin;
                Vec2<int> advance;
                Bbox<int> bbox;
                FT_UInt glyph_i;
            };

            // font page
            struct Page
            {
                GLuint tex;
                Char_info char_info[256];
            };

            // create a font page texture
            std::unordered_map<uint32_t, Page>::iterator load_page(const uint32_t page_no);

            // font data
            FT_Face _face;
            bool _has_kerning_info;
            Bbox<int> _cell_bbox;
            int _line_height;

            // texture size
            size_t _tex_width, _tex_height;

            // font pages
            std::unordered_map<uint32_t, Page> _page_map;

            // OpenGL vertex object
            GLuint _vao;
            GLuint _vbo;

            friend class Static_text;
            friend std::pair<std::vector<textogl::Vec2<float>>, std::vector<textogl::Font_sys::Coord_data>> build_text(
                    const std::string & utf8_input,
                    textogl::Font_sys & font_sys,
                    textogl::Font_sys::Bbox<float> & font_box_out);
    };
}

#endif // FONT_HPP
