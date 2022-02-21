/// @file
/// @brief Font loading and text display

// Copyright 2022 Matthew Chandler

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
#include <vector>

#include "types.hpp"

/// @ingroup textogl
namespace textogl
{
    /// Text origin specification
    enum Text_origin: int
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
                 const unsigned int font_size   ///< Font size (in pixels)
                 );
        /// Load a font at a specified size from memory

        /// data is not copied, so the client is responsible for maintaining the data for the lifetime of this object
        Font_sys(const unsigned char * font_data,  ///< Font file data (in memory)
                 const std::size_t font_data_size, ///< Font file data's size in memory
                 const unsigned int font_size      ///< Font size (in pixels)
                 );

        /// Resize font

        /// Resizes the font without destroying it
        /// @note This will require rebuilding font textures
        /// @note Any Static_text objects tied to this Font_sys will need to have Static_text::set_font_sys called
        void resize(const unsigned int font_size ///< Font size (in pixels)
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

        /// Render given text, with rotatation

        /// Renders the text supplied in utf8_input parameter
        /// @note This will rebuild the OpenGL primitives each call.
        /// If the text will not change frequently, use a Static_text object
        /// instead
        void render_text_rotate(const std::string & utf8_input, ///< Text to render, in UTF-8 encoding. For best performance, normalize the string before rendering
                                const Color & color,            ///< Text Color
                                const Vec2<float> & win_size,   ///< Window dimensions. A Vec2 with X = width and Y = height
                                const Vec2<float> & pos,        ///< Render position, in screen pixels
                                const float rotation,           ///< Clockwise text rotation (in radians) around origin as defined in align_flags. 0 is vertical
                                const int align_flags = 0       ///< Text Alignment. Should be #Text_origin flags bitwise-OR'd together
                                );

        /// Render given text, using a model view projection matrix

        /// Renders the text supplied in utf8_input parameter, using a model view projection matrix
        /// @note This will rebuild the OpenGL primitives each call.
        /// If the text will not change frequently, use a Static_text object
        /// instead
        void render_text_mat(const std::string & utf8_input,       ///< Text to render, in UTF-8 encoding. For best performance, normalize the string before rendering
                             const Color & color,                  ///< Text Color
                             /// Model view projection matrix.
                             /// The text will be rendered as quads, one for each glyph, with vertex coordinates centered on the baselines and sized in pixels.
                             /// This matrix will be used to transform that geometry
                             const Mat4<float> & model_view_projection
                             );

    private:
        struct Impl; ///< Private internal implementation
        std::shared_ptr<Impl> pimpl; ///< Pointer to private internal implementation

        /// @cond INTERNAL
        friend class Static_text;
        /// @endcond
    };
}

#endif // FONT_HPP
