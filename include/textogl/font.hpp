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

#ifdef USE_GLM
#include <glm/glm.hpp>
#endif

#include <ft2build.h>
#include FT_FREETYPE_H

/// OpenGL Font rendering types

/// @ingroup textogl
namespace textogl
{
    /// %Color vector

    /// Simple RGBA color vector
#ifdef USE_GLM
    using Color = glm::vec4;
#else
    struct Color
    {
        float r; ///< Red component value
        float g; ///< Green component value
        float b; ///< Blue component value
        float a; ///< Alpha component value

        /// Access component by index

        /// To pass color to OpenGL, do: <tt>&color[0]</tt>
        /// @{
        float & operator[](std::size_t i) { return (&r)[i]; }
        const float & operator[](std::size_t i) const { return (&r)[i]; }
        /// @}
    };
#endif

    /// 2D Vector
    namespace detail
    {
        template<typename T>
        struct Vec2
        {
            T x; ///< X component
            T y; ///< Y component

            /// Access component by index

            /// To pass vector to OpenGL, do: <tt>&vec2[0]</tt>
            /// @{
            float & operator[](std::size_t i) { return (&x)[i]; }
            const float & operator[](std::size_t i) const { return (&x)[i]; }
            /// @}
        };

        template <typename T> struct Generic_vec2 { using t = Vec2<T>; };
#ifdef USE_GLM
        template<> struct Generic_vec2<bool>         { using t =  glm::bvec2; };
        template<> struct Generic_vec2<double>       { using t =  glm::dvec2; };
        template<> struct Generic_vec2<int>          { using t =  glm::ivec2; };
        template<> struct Generic_vec2<unsigned int> { using t =  glm::uvec2; };
        template<> struct Generic_vec2<float>        { using t =  glm::vec2;  };
#endif
    };
    template<typename T> using Vec2    = typename detail::Generic_vec2<T>::t;

    /// Text origin specification
    enum Text_origin
    {
        ORIGIN_HORIZ_BASELINE = 0x00, ///< Horizontal text origin at baseline
        ORIGIN_HORIZ_LEFT     = 0x01, ///< Horizontal text origin at left edge
        ORIGIN_HORIZ_RIGHT    = 0x02, ///< Horizontal text origin at right edge
        ORIGIN_HORIZ_CENTER   = 0x03, ///< Horizontal text origin at center

        ORIGIN_VERT_BASELINE  = 0x00, ///< Vertical text origin at baseline
        ORIGIN_VERT_TOP       = 0x04, ///< Vertical text origin at left edge
        ORIGIN_VERT_BOTTOM    = 0x08, ///< Vertical text origin at right edge
        ORIGIN_VERT_CENTER    = 0x0C  ///< Vertical text origin at center
    };

    /// Container for font and text rendering

    /// Contains everything needed for rendering from the specified font at the
    /// specified size. Unicode is supported for all glyphs provided by the
    /// specified font.
    ///
    /// Rendering data is created by unicode code page (block of 256 code-points),
    /// as it is used. Only those pages that are used are built. The 1st page
    /// (Basic Latin and Latin-1 Supplement) is always created.
    class Font_sys
    {
        public:
            /// Load a font file at a specified size
            Font_sys(const std::string & font_path, ///< Path to font file to use
                     const unsigned int font_size,  ///< Font size (in points)
                     const unsigned int v_dpi = 96, ///< Font vertical DPI
                     const unsigned int h_dpi = 96  ///< Font horizontal DPI
                     );
            ~Font_sys();

            /// @name Non-copyable
            /// @{
            Font_sys(const Font_sys &) = delete;
            Font_sys & operator=(const Font_sys &) = delete;
            /// @}

            /// @name Movable
            /// @{
            Font_sys(Font_sys &&);
            Font_sys & operator=(Font_sys &&);
            /// @}

            /// Resize font

            /// Resizes the font without destroying it
            /// @note This will require rebuilding font textures
            /// @note Any Static_text objects tied to this Font_sys will need to have Static_text::set_font_sys called
            void resize(const unsigned int font_size,  ///< Font size (in points)
                        const unsigned int v_dpi = 96, ///< Font vertical DPI
                        const unsigned int h_dpi = 96  ///< Font horizontal DPI
                        );

            /// Render given text

            /// Renders the text supplied in utf8_input parameter
            /// @note This will rebuild the OpenGL primitives each call.
            /// If the text will not change frequently, use a Static_text object
            /// instead
            void render_text(const std::string & utf8_input, ///< Text to render, in UTF-8 encoding. For best performance, normalize the string before rendering
                             const Color & color,            ///< Text Color
                             const Vec2<float> & win_size,   ///< Window dimensions. A Vec2 with X = width and Y = height
                             const Vec2<float> & pos,        ///< Render position, in screen pixels
                             const int align_flags = 0       ///< Text Alignment. Should be #Text_origin flags bitwise-OR'd together
                             );

        private:

            /// Container for Freetype library object and shader program

            /// Every Font_sys object can use the same instance of Font_Common,
            /// so only one should ever be initialized at any given time.
            /// Access to the single instance provied through Font_sys::_common_data
            class Font_common
            {
            public:
                Font_common();
                ~Font_common();

                /// @name Non-copyable, non-movable
                /// @{
                Font_common(const Font_common &) = delete;
                Font_common & operator=(const Font_common &) = delete;

                Font_common(Font_common &&) = delete;
                Font_common & operator=(Font_common &&) = delete;
                /// @}

                FT_Library ft_lib; ///< Freetype library object. [See Freetype documentation](https://www.freetype.org/freetype2/docs/reference/ft2-base_interface.html#FT_Library)
                GLuint prog;       ///< OpenGL shader program index
                std::unordered_map<std::string, GLuint> uniform_locations; ///< OpenGL shader program uniform location indexes
            };

            /// Bounding box

            /// Used for laying out text and defining character and text boundaries
            template<typename T>
            struct Bbox
            {
                Vec2<T> ul; ///< Upper-left corner
                Vec2<T> lr; ///< Lower-right corner

                /// Get the width of the box
                T width() const
                {
                    return lr.x - ul.x;
                }

                /// Get the height of the box
                T height() const
                {
                    return ul.y - lr.y;
                }
            };

            /// OpenGL Vertex buffer object data
            struct Coord_data
            {
                uint32_t page_no;         ///< Unicode code page number for a set of characters
                std::size_t start;        ///< Starting index into \ref _vbo for this pages's quads
                std::size_t num_elements; ///< Number of indexs to render for this page
            };

            /// Character info

            /// Contains information about a single code-point (character)
            struct Char_info
            {
                Vec2<int> origin;  ///< Character's origin within \ref bbox
                Vec2<int> advance; ///< Distance to next char's origin
                Bbox<int> bbox;    ///< Bounding box for the character
                FT_UInt glyph_i;
            };

            /// Font page

            /// Texture for a single Unicode code 'page' (where a page is 256
            /// consecutive code points)
            struct Page
            {
                GLuint tex;               ///< OpenGL Texture index for the page
                Char_info char_info[256]; ///< Info for each code point on the page
            };

            /// Create data for a code page

            /// Creates texture and layout information for the given code page
            /// @param page_no The Unicode page number to build
            /// @returns iterator into \ref _page_map containing the page data
            /// @note This assumes the page hasn't been created yet. Do not call
            ///       if the page already exists in \ref _page_map
            std::unordered_map<uint32_t, Page>::iterator load_page(const uint32_t page_no);

            /// Common font rendering routine

            /// Rendering calls common to Font_sys and Static_text
            void render_text_common(const Color & color,                        ///< Text Color
                                    const Vec2<float> & win_size,               ///< Window dimensions. A Vec2 with X = width and Y = height
                                    const Vec2<float> & pos,                    ///< Render position, in screen pixels
                                    const int align_flags,                      ///< Text Alignment. Should be #Text_origin flags bitwise-OR'd together
                                    const Bbox<float> & text_box,               ///< Text's bounding box
                                    const std::vector<Coord_data> & coord_data, ///< Pre-calculated coordinate data as returned by \ref build_text
                                    GLuint vao                                  ///< OpenGL vertex array object
                            );

            /// Build buffer of quads for and coordinate data for text display

            /// @param utf8_input Text to build data for
            /// @returns A tuple of
            /// * Quad coordinates, ready to be stored into an OpenGL VBO
            /// * VBO start and end data for use in glDrawArrays
            /// * Bounding box of resulting text
            std::tuple<std::vector<Vec2<float>>, std::vector<Coord_data>, Bbox<float>>
            build_text(const std::string & utf8_input);

            static std::unique_ptr<Font_common> _common_data; ///< Font data common to all instances of Font_sys
            static unsigned int _common_ref_cnt; ///< Reference count for \ref _common_data

            /// @name Font data
            /// @{
            FT_Face _face;          ///< Font face. [See Freetype documentation](https://www.freetype.org/freetype2/docs/reference/ft2-base_interface.html#FT_Face)
            bool _has_kerning_info; ///< \c true if the font has kerning information available
            Bbox<int> _cell_bbox;   ///< Bounding box representing maximum extents of a glyph
            int _line_height;       ///< Spacing between baselines for each line of text
            /// @}

            /// @name Texture size
            /// Width and height of the texture. Each font page will be rendered to a
            /// grid of 16x16 glyphs, with each cell in the grid being
            /// \ref _cell_bbox sized + 2px for padding
            /// @{
            size_t _tex_width;
            size_t _tex_height;
            /// @}

            std::unordered_map<uint32_t, Page> _page_map; ///< Font pages

            GLuint _vao; ///< OpenGL Vertex array object index
            GLuint _vbo; ///< OpenGL Vertex buffer object index

            /// @cond INTERNAL
            friend class Static_text;
            /// @endcond
    };
}

#endif // FONT_HPP
