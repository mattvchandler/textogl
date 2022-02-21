/// @file
/// @brief Font loading and text display implementation

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

#include "font_impl.hpp"

#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <system_error>

#include <cmath>

/// Convert a UTF-8 string to a UTF-32 string

/// Minimal verification is done to ensure that the input is valid UTF-8. If any
/// invalid byte sequences are detected, they will be replaced by the replacement
/// character: '�'
/// @param utf8 UTF-8 string to convert
/// @returns UTF-32 string corresponding to the input
std::u32string utf8_to_utf32(const std::string & utf8)
{
    std::u32string utf32;

    uint32_t code_pt = 0;
    int expected_bytes = 0;

    for(const uint8_t byte: utf8)
    {
        // detect invalid bytes
        if(byte == 0xC0 || byte == 0xC1 || byte >= 0xF5)
        {
            utf32.push_back(U'�');
            expected_bytes = 0;
        }
        // 0b0xxxxxxx: single-byte char (ASCII)
        else if((byte & 0x80) == 0)
        {
            if(expected_bytes != 0)
            {
                // previous sequence ended prematurely. add replacement char
                utf32.push_back(U'�');
                expected_bytes = 0;
            }
            utf32.push_back(byte);
        }
        // 0b11xxxxxx: leading byte
        else if((byte & 0xC0) == 0xC0)
        {
            if(expected_bytes != 0)
            {
                // previous sequence ended prematurely. add replacement char
                utf32.push_back(U'�');
            }

            // 2-byte char
            if((byte & 0xE0) == 0xC0)
            {
                code_pt = byte & 0x1F;
                expected_bytes = 1;
            }
            // 3-byte char
            else if((byte & 0xF0) == 0xE0)
            {
                code_pt = byte & 0x0F;
                expected_bytes = 2;
            }
            // 4-byte char
            else if((byte & 0xF8) == 0xF0)
            {
                code_pt = byte & 0x07;
                expected_bytes = 3;
            }
            else
            {
                // invalid value. insert the replacement char
                utf32.push_back(U'�');
                expected_bytes = 0;
            }
        }
        // 0b10xxxxxx: continuation byte
        else // (byte & 0xC0) == 0x80
        {
            if(expected_bytes == 0)
            {
                // continuation byte w/o leader. replace w/ replacement char
                utf32.push_back(U'�');
            }
            else
            {
                code_pt <<= 6;
                code_pt |= byte & 0x3F;

                if(--expected_bytes == 0)
                {
                    utf32.push_back(code_pt);
                }
            }
        }
    }

    if(expected_bytes > 0)
    {
        // end of string but still expecting continuation bytes. use the replacement char
        utf32.push_back(U'�');
    }

    return utf32;
}

namespace textogl
{
    Font_sys::Font_sys(const std::string & font_path, const unsigned int font_size):
        pimpl(std::make_shared<Impl>(font_path, font_size))
    {}
    Font_sys::Impl::Impl(const std::string & font_path, const unsigned int font_size)
    {
        FT_Open_Args args
        {
            FT_OPEN_PATHNAME,
            nullptr,
            0,
            const_cast<char *>(font_path.c_str()),
            nullptr,
            nullptr,
            0,
            nullptr
        };
        init(args, font_size);
    }

    Font_sys::Font_sys(const unsigned char * font_data, std::size_t font_data_size, const unsigned int font_size):
        pimpl(std::make_shared<Impl>(font_data, font_data_size, font_size))
    {}
    Font_sys::Impl::Impl(const unsigned char * font_data, std::size_t font_data_size, const unsigned int font_size)
    {
        FT_Open_Args args
        {
            FT_OPEN_MEMORY,
            font_data,
            static_cast<FT_Long>(font_data_size),
            nullptr,
            nullptr,
            nullptr,
            0,
            nullptr
        };
        init(args, font_size);
    }

    void Font_sys::Impl::init(FT_Open_Args & args, const unsigned int font_size)
    {
        // load freetype, and text shader - only once
        if(common_ref_cnt_ == 0)
            common_data_.reset(new Font_common);

        // open the font file
        FT_Error err = FT_Open_Face(common_data_->ft_lib, &args, 0, &face_);

        if(err != FT_Err_Ok)
        {
            if(common_ref_cnt_ == 0)
                common_data_.reset();

            if(err == FT_Err_Unknown_File_Format)
            {
                throw std::system_error(err, std::system_category(), "Unknown format for font file");
            }
            else
            {
                throw std::ios_base::failure("Error reading font file");
            }
        }

        // select unicode charmap (should be default for most fonts)
        if(FT_Select_Charmap(face_, FT_ENCODING_UNICODE) != FT_Err_Ok)
        {
            FT_Done_Face(face_);

            if(common_ref_cnt_ == 0)
                common_data_.reset();

            throw std::runtime_error("No unicode charmap in font file");
        }

        try
        {
            resize(font_size);
        }
        catch(std::runtime_error &)
        {
            FT_Done_Face(face_);

            if(common_ref_cnt_ == 0)
                common_data_.reset();

            throw;
        }

        // we're not going to throw now, so increment library ref count
        ++common_ref_cnt_;

        // create and set up vertex array and buffer
        glGenVertexArrays(1, &vao_);
        glBindVertexArray(vao_);

        glGenBuffers(1, &vbo_);
        glBindBuffer(GL_ARRAY_BUFFER, vbo_);

        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(Vec2<float>), NULL);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(Vec2<float>), (const GLvoid *)sizeof(Vec2<float>));
        glEnableVertexAttribArray(1);
        glBindVertexArray(0);

        glBindBuffer(GL_ARRAY_BUFFER, 0);

        glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &max_tu_count_);
        max_tu_count_--;

        // get shader uniform locations
        glUseProgram(common_data_->prog);
        glUniform1i(common_data_->uniform_locations["font_page"], max_tu_count_);
        glUseProgram(0);
    }

    Font_sys::Impl::~Impl()
    {
        FT_Done_Face(face_);

        // only deallocate shared libs if this is the last Font_sys obj
        if(--common_ref_cnt_ == 0)
            common_data_.reset();

        // destroy VAO/VBO
        glDeleteBuffers(1, &vbo_);
        glDeleteVertexArrays(1, &vao_);

        // destroy textures
        for(auto & i: page_map_)
        {
            glDeleteTextures(1, &i.first);
        }
    }

    Font_sys::Impl::Impl(Impl && other):
        face_(other.face_),
        has_kerning_info_(other.has_kerning_info_),
        cell_bbox_(std::move(other.cell_bbox_)),
        line_height_(other.line_height_),
        tex_width_(other.tex_width_),
        tex_height_(other.tex_height_),
        page_map_(std::move(other.page_map_)),
        vao_(other.vao_),
        vbo_(other.vbo_)
    {
        other.face_ = nullptr;
        other.vao_ = 0;
        other.vbo_ = 0;
        ++common_ref_cnt_;
    }
    Font_sys::Impl & Font_sys::Impl::operator=(Impl && other)
    {
        if(this != &other)
        {
            face_ = other.face_;
            has_kerning_info_ = other.has_kerning_info_;
            cell_bbox_ = std::move(other.cell_bbox_);
            line_height_ = other.line_height_;
            tex_width_ = other.tex_width_;
            tex_height_ = other.tex_height_;
            page_map_ = std::move(other.page_map_);
            vao_ = other.vao_;
            vbo_ = other.vbo_;

            other.face_ = nullptr;
            other.vao_ = 0;
            other.vbo_ = 0;
        }
        return *this;
    }

    void Font_sys::resize(const unsigned int font_size)
    {
        pimpl->resize(font_size);
    }
    void Font_sys::Impl::resize(const unsigned int font_size)
    {
        // select font size
        if(FT_Set_Pixel_Sizes(face_, 0, font_size) != FT_Err_Ok)
            throw std::runtime_error("Can't set font size: " + std::to_string(font_size));

        // get bounding box that will fit any glyph, plus 2 px padding
        // some glyphs overflow the reported box (antialiasing?) so at least one px is needed
        cell_bbox_.ul.x = FT_MulFix(face_->bbox.xMin, face_->size->metrics.x_scale) / 64 - 2;
        cell_bbox_.ul.y = FT_MulFix(face_->bbox.yMax, face_->size->metrics.y_scale) / 64 + 2;
        cell_bbox_.lr.x = FT_MulFix(face_->bbox.xMax, face_->size->metrics.x_scale) / 64 + 2;
        cell_bbox_.lr.y = FT_MulFix(face_->bbox.yMin, face_->size->metrics.y_scale) / 64 - 2;

        // get newline height
        line_height_ = FT_MulFix(face_->height, face_->size->metrics.y_scale) / 64;

        tex_width_ = cell_bbox_.width() * 16;
        tex_height_ = cell_bbox_.height() * 16;

        has_kerning_info_ = FT_HAS_KERNING(face_);

        page_map_.clear();
    }

    void Font_sys::render_text(const std::string & utf8_input, const Color & color,
            const Vec2<float> & win_size, const Vec2<float> & pos, const int align_flags)
    {
        pimpl->render_text(utf8_input, color, win_size, pos, 0.0f, align_flags);
    }
    void Font_sys::render_text_rotate(const std::string & utf8_input, const Color & color,
            const Vec2<float> & win_size, const Vec2<float> & pos, const float rotation,
            const int align_flags)
    {
        pimpl->render_text(utf8_input, color, win_size, pos, rotation, align_flags);
    }
    void Font_sys::Impl::render_text(const std::string & utf8_input, const Color & color,
            const Vec2<float> & win_size, const Vec2<float> & pos, const float rotation,
            const int align_flags)
    {
        // build text buffer objs
        std::vector<Vec2<float>> coords;
        std::vector<Font_sys::Impl::Coord_data> coord_data;
        Font_sys::Impl::Bbox<float> text_box;
        std::tie(coords, coord_data, text_box) = build_text(utf8_input);

        load_text_vbo(coords);

        render_text_common(color, win_size, pos, align_flags, rotation, text_box, coord_data, vao_, vbo_);
    }

    void Font_sys::Impl::render_text_common(const Color & color, const Vec2<float> & win_size,
            const Vec2<float> & pos, const int align_flags, const float rotation,
            const Bbox<float> & text_box, const std::vector<Coord_data> & coord_data,
             GLuint vao, GLuint vbo)
    {
        Vec2<float> start_offset{0.0f, 0.0f};

        // offset origin to align to text bounding box
        int horiz_align = align_flags & 0x3;
        switch(horiz_align)
        {
            case ORIGIN_HORIZ_BASELINE:
                break;
            case ORIGIN_HORIZ_LEFT:
                start_offset.x = text_box.ul.x;
                break;
            case ORIGIN_HORIZ_RIGHT:
                start_offset.x = text_box.lr.x;
                break;
            case ORIGIN_HORIZ_CENTER:
                start_offset.x = text_box.ul.x + text_box.width() / 2.0f;
                break;
        }

        int vert_align = align_flags & 0xC;
        switch(vert_align)
        {
            case ORIGIN_VERT_BASELINE:
                break;
            case ORIGIN_VERT_TOP:
                start_offset.y = text_box.ul.y;
                break;
            case ORIGIN_VERT_BOTTOM:
                start_offset.y = text_box.lr.y;
                break;
            case ORIGIN_VERT_CENTER:
                start_offset.y = text_box.lr.y + text_box.height() / 2.0f;
                break;
        }

        // this is the result of multiplying matrices as follows:
        // projection(0, win_size.x, win_size.y, 0) * translate(pos) * rotate(rotation, {0,0,1}) * translate(-start_offset)
        Mat4<float> model_view_projection
        {
            2.0f * std::cos(rotation) / win_size.x, -2.0f * std::sin(rotation) / win_size.y, 0.0f, 0.0f,
           -2.0f * std::sin(rotation) / win_size.x, -2.0f * std::cos(rotation) / win_size.y, 0.0f, 0.0f,
            0.0f,                                    0.0f,                                   1.0f, 0.0f,

           -1.0f + 2.0f * (pos.x - std::cos(rotation) * start_offset.x + std::sin(rotation) * start_offset.y) / win_size.x,
                        1.0f - 2.0f * (pos.y - std::sin(rotation) * start_offset.x - std::cos(rotation) * start_offset.y) / win_size.y,
                        0.0f, 1.0f
        };

        render_text_common(color, model_view_projection, coord_data, vao, vbo);
    }

    void Font_sys::render_text_mat(const std::string & utf8_input, const Color & color, const Mat4<float> & model_view_projection)
    {
        pimpl->render_text(utf8_input, color, model_view_projection);
    }
    void Font_sys::Impl::render_text(const std::string & utf8_input, const Color & color, const Mat4<float> & model_view_projection)
    {
        std::vector<Vec2<float>> coords;
        std::vector<Font_sys::Impl::Coord_data> coord_data;
        std::tie(coords, coord_data, std::ignore) = build_text(utf8_input);

        render_text_common(color, model_view_projection, coord_data, vao_, vbo_);
    }

    void Font_sys::Impl::render_text_common(const Color & color, const Mat4<float> & model_view_projection,
            const std::vector<Coord_data> & coord_data, GLuint vao, GLuint vbo)
    {
        // save old settings
        GLint old_vao{0}, old_vbo{0}, old_prog{0};
        GLint old_blend_src{0}, old_blend_dst{0};
        GLint old_active_texture{0}, old_texture_2d{0};

        glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &old_vao);
        glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &old_vbo);
        glGetIntegerv(GL_CURRENT_PROGRAM, &old_prog);
        glGetIntegerv(GL_BLEND_SRC_RGB, &old_blend_src);
        glGetIntegerv(GL_BLEND_DST_RGB, &old_blend_dst);
        glGetIntegerv(GL_ACTIVE_TEXTURE, &old_active_texture);
        glGetIntegerv(GL_TEXTURE_BINDING_2D, &old_texture_2d);

        auto old_depth_test = glIsEnabled(GL_DEPTH_TEST);
        auto old_blend = glIsEnabled(GL_BLEND);

        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);

        // set up shader uniforms
        glUseProgram(common_data_->prog);
        glUniformMatrix4fv(common_data_->uniform_locations["model_view_projection"], 1, GL_FALSE, &model_view_projection[0][0]);
        glUniform4fv(common_data_->uniform_locations["color"], 1, &color[0]);

        glDisable(GL_DEPTH_TEST);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glActiveTexture(GL_TEXTURE0 + max_tu_count_);

        // draw text, per page
        for(const auto & cd: coord_data)
        {
            // bind the page's texture
            glBindTexture(GL_TEXTURE_2D, page_map_[cd.page_no].tex);
            glDrawArrays(GL_TRIANGLES, cd.start, cd.num_elements);
        }

        // restore old settings
        glBindVertexArray(old_vao);
        glBindBuffer(GL_ARRAY_BUFFER, old_vbo);
        glUseProgram(old_prog);

        if(old_depth_test)
            glEnable(GL_DEPTH_TEST);
        else
            glDisable(GL_DEPTH_TEST);

        if(old_blend)
            glEnable(GL_BLEND);
        else
            glDisable(GL_BLEND);

        glBlendFunc(old_blend_src, old_blend_dst);
        glActiveTexture(old_active_texture);
        glBindTexture(GL_TEXTURE_2D, old_texture_2d);
    }

    std::unordered_map<uint32_t, Font_sys::Impl::Page>::iterator Font_sys::Impl::load_page(const uint32_t page_no)
    {
        // this assumes the page has not been created yet
        auto page_i = page_map_.emplace(std::make_pair(page_no, Page())).first;
        Page & page = page_i->second;

        // greyscale pixel storage
        std::vector<char> tex_data(tex_width_ * tex_height_, 0);

        FT_GlyphSlot slot = face_->glyph;

        // load each glyph in the page (256 per page)
        for(uint32_t code_pt = page_no << 8; code_pt < ((page_no + 1) << 8); code_pt++)
        {
            unsigned short tbl_row = (code_pt >> 4) & 0xF;
            unsigned short tbl_col = code_pt & 0xF;

            // have freetype render the glyph
            FT_UInt glyph_i = FT_Get_Char_Index(face_, code_pt);
            if(FT_Load_Glyph(face_, glyph_i, FT_LOAD_RENDER) != FT_Err_Ok)
            {
                std::cerr<<"Err loading glyph for: "<<std::hex<<std::showbase<<code_pt;
                continue;
            }

            FT_Bitmap * bmp = &slot->bitmap;
            Char_info & c = page.char_info[code_pt & 0xFF];

            // set glyph properties
            c.origin.x = -cell_bbox_.ul.x + slot->bitmap_left;
            c.origin.y = cell_bbox_.ul.y - slot->bitmap_top;
            c.bbox.ul.x = slot->bitmap_left;
            c.bbox.ul.y = slot->bitmap_top;
            c.bbox.lr.x = (int)bmp->width + slot->bitmap_left;
            c.bbox.lr.y = slot->bitmap_top - (int)bmp->rows;
            c.advance.x = slot->advance.x;
            c.advance.y = slot->advance.y;
            c.glyph_i = glyph_i;

            // copy glyph from freetype to texture storage
            // TODO: we are assuming bmp->pixel_mode == FT_PIXEL_MODE_GRAY here.
            // We will probably want to allow other formats at some point
            for(std::size_t y = 0; y < (std::size_t)bmp->rows; ++y)
            {
                for(std::size_t x = 0; x < (std::size_t)bmp->width; ++x)
                {
                    long tbl_img_y = tbl_row * cell_bbox_.height() + cell_bbox_.ul.y - slot->bitmap_top + y;
                    long tbl_img_x = tbl_col * cell_bbox_.width() - cell_bbox_.ul.x + slot->bitmap_left + x;

                    tex_data[tbl_img_y * tex_width_ + tbl_img_x] = bmp->buffer[y * bmp->width + x];
                }
            }
        }

        // copy data to a new opengl texture
        glGenTextures(1, &page.tex);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, page.tex);
#ifndef USE_OPENGL_ES
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, tex_width_, tex_height_, 0, GL_RED, GL_UNSIGNED_BYTE, tex_data.data());
#else
        glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, tex_width_, tex_height_, 0, GL_ALPHA, GL_UNSIGNED_BYTE, tex_data.data());
#endif

        glGenerateMipmap(GL_TEXTURE_2D);
        // set params
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        return page_i;
    }

    std::tuple<std::vector<Vec2<float>>, std::vector<Font_sys::Impl::Coord_data>, Font_sys::Impl::Bbox<float>>
    Font_sys::Impl::build_text(const std::string & utf8_input)
    {
        Vec2<float> pen{0.0f, 0.0f};

        // verts by font page
        std::unordered_map<uint32_t, std::vector<Vec2<float>>> screen_and_tex_coords;

        Bbox<float> font_box;

        font_box.ul.x = std::numeric_limits<float>::max();
        font_box.ul.y = std::numeric_limits<float>::max();
        font_box.lr.x = std::numeric_limits<float>::min();
        font_box.lr.y = std::numeric_limits<float>::min();

        FT_UInt prev_glyph_i = 0;

        for(auto & code_pt : utf8_to_utf32(utf8_input))
        {
            // handle newlines
            if(code_pt == '\n')
            {
                pen.x = 0;
                pen.y += line_height_;
                prev_glyph_i = 0;
                continue;
            }

            // get font page struct
            uint32_t page_no = code_pt >> 8;
            auto page_i = page_map_.find(page_no);

            // load page if not already loaded
            if(page_i == page_map_.end())
            {
                page_i = load_page(code_pt >> 8);
            }

            Font_sys::Impl::Page & page = page_i->second;
            Font_sys::Impl::Char_info & c = page.char_info[code_pt & 0xFF];

            // add kerning if necessary
            if(has_kerning_info_ && prev_glyph_i && c.glyph_i)
            {
                FT_Vector kerning = {0, 0};
                if(FT_Get_Kerning(face_, prev_glyph_i, c.glyph_i, FT_KERNING_DEFAULT, &kerning) != FT_Err_Ok)
                {
                    std::cerr<<"Can't load kerning for: "<<std::hex<<std::showbase<<code_pt;
                }
                pen.x += kerning.x / 64.0f;
                pen.y -= kerning.y / 64.0f;
            }

            std::size_t tex_row = (code_pt >> 4) & 0xF;
            std::size_t tex_col = code_pt & 0xF;

            // texture coord of glyph's origin
            Vec2<float> tex_origin = {(float)(tex_col * cell_bbox_.width() - cell_bbox_.ul.x),
                (float)(tex_row * cell_bbox_.height() + cell_bbox_.ul.y)};

            // push back vertex coords, and texture coords, interleaved, into a map by font page
            // 1 unit to pixel scale
            // lower left corner
            screen_and_tex_coords[page_no].push_back({pen.x + c.bbox.ul.x,
                    pen.y - c.bbox.lr.y});
            screen_and_tex_coords[page_no].push_back({(tex_origin.x + c.bbox.ul.x) / tex_width_,
                    (tex_origin.y - c.bbox.lr.y) / tex_height_});
            // lower right corner
            screen_and_tex_coords[page_no].push_back({pen.x + c.bbox.lr.x,
                    pen.y - c.bbox.lr.y});
            screen_and_tex_coords[page_no].push_back({(tex_origin.x + c.bbox.lr.x) / tex_width_,
                    (tex_origin.y - c.bbox.lr.y) / tex_height_});
            // upper left corner
            screen_and_tex_coords[page_no].push_back({pen.x + c.bbox.ul.x,
                    pen.y - c.bbox.ul.y});
            screen_and_tex_coords[page_no].push_back({(tex_origin.x + c.bbox.ul.x) / tex_width_,
                    (tex_origin.y - c.bbox.ul.y) / tex_height_});

            // upper left corner
            screen_and_tex_coords[page_no].push_back({pen.x + c.bbox.ul.x,
                    pen.y - c.bbox.ul.y});
            screen_and_tex_coords[page_no].push_back({(tex_origin.x + c.bbox.ul.x) / tex_width_,
                    (tex_origin.y - c.bbox.ul.y) / tex_height_});
            // lower right corner
            screen_and_tex_coords[page_no].push_back({pen.x + c.bbox.lr.x,
                    pen.y - c.bbox.lr.y});
            screen_and_tex_coords[page_no].push_back({(tex_origin.x + c.bbox.lr.x) / tex_width_,
                    (tex_origin.y - c.bbox.lr.y) / tex_height_});
            // upper right corner
            screen_and_tex_coords[page_no].push_back({pen.x + c.bbox.lr.x,
                    pen.y - c.bbox.ul.y});
            screen_and_tex_coords[page_no].push_back({(tex_origin.x + c.bbox.lr.x) / tex_width_,
                    (tex_origin.y - c.bbox.ul.y) / tex_height_});

            // expand bounding box for whole string
            font_box.ul.x = std::min(font_box.ul.x, pen.x + c.bbox.ul.x);
            font_box.ul.y = std::min(font_box.ul.y, pen.y - c.bbox.ul.y);
            font_box.lr.x = std::max(font_box.lr.x, pen.x + c.bbox.lr.x);
            font_box.lr.y = std::max(font_box.lr.y, pen.y - c.bbox.lr.y);

            // advance to next origin
            pen.x += c.advance.x / 64.0f;
            pen.y -= c.advance.y / 64.0f;

            prev_glyph_i = c.glyph_i;
        }

        // reorganize texture data into a contiguous array
        std::vector<Vec2<float>> coords;
        std::vector<Font_sys::Impl::Coord_data> coord_data;


        for(const auto & page: screen_and_tex_coords)
        {
            coord_data.emplace_back();
            Font_sys::Impl::Coord_data & c = coord_data.back();

            c.page_no = page.first;

            c.start = coords.size() / 2;
            coords.insert(coords.end(), page.second.begin(), page.second.end());
            c.num_elements = coords.size() / 2 - c.start;
        }

        return std::make_tuple(coords, coord_data, font_box);
    }

    void Font_sys::Impl::load_text_vbo(const std::vector<Vec2<float>> & coords) const
    {
        glBindBuffer(GL_ARRAY_BUFFER, vbo_);

        // load text into buffer object
        // call glBufferData with NULL first - this is apparently faster for dynamic data loading
        glBufferData(GL_ARRAY_BUFFER, sizeof(Vec2<float>) * coords.size(), NULL, GL_DYNAMIC_DRAW);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(Vec2<float>) * coords.size(), coords.data());

    }
}
