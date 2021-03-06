//
// Created by luiz0tavio on 5/27/18.
//

#include "BulkText.hpp"

Renderer::BulkText &Renderer::BulkText::getInstance()
{
    static Renderer::BulkText instance;
    return instance;
}

void * Renderer::BulkText::operator new (std::size_t size)
{
    return Memory::Provider::getMemory(Memory::PoolType::POOL_TYPE_GENERIC, size);
}

Renderer::BulkText::BulkText() : texts_({}), freetype_face_(nullptr)
{
    FT_Library ft;

    if (FT_Init_FreeType(&ft)) {
        fprintf(stderr, "Could not init freetype library\n");
    }

    // @TODO Allow set a font for each Text
    if (FT_New_Face(ft, "data/fonts/VanillaGalaxies.ttf", 0, &this->freetype_face_)) {
        fprintf(stderr, "Could not open font\n");
    }

    // Compile/Link/Set Shader Program
    this->shader_ = new Renderer::Shader();
    this->shader_->createGraphicShader(GL_VERTEX_SHADER, "text.vert");
    this->shader_->createGraphicShader(GL_FRAGMENT_SHADER, "text.frag");
    this->shader_->link();

    // Get Shader positions
    GLint loc;

    loc = glGetUniformLocation(this->shader_->getShaderProgram(), "tex");
    if (loc < 0) std::cerr << "Can't find 'tex' uniform on shader!" << std::endl;
    this->shader_tex_pos_ = static_cast<GLuint>(loc);

    loc = glGetAttribLocation(this->shader_->getShaderProgram(), "coord");
    if (loc < 0) std::cerr << "Can't find 'coord' attr on shader!" << std::endl;
    this->shader_coord_pos_ = static_cast<GLuint>(loc);

    loc = glGetUniformLocation(this->shader_->getShaderProgram(), "color");
    if (loc < 0) std::cerr << "Can't find 'color' uniform on shader!" << std::endl;
    this->shader_color_pos_ = static_cast<GLuint>(loc);
}

void Renderer::BulkText::push_back(Text* text)
{
    texts_.push_back(text);
}

GLuint Renderer::BulkText::getShaderProgram()
{
    return this->shader_->getShaderProgram();
}

void Renderer::BulkText::draw(Renderer::Camera* camera)
{
    // Init Text Shader
    this->shader_->use();

    FT_GlyphSlot g = this->freetype_face_->glyph;

    GLuint textures;

    /* Create a texture that will be used to hold one "glyph" */
    glActiveTexture(GL_TEXTURE0);
    glGenTextures(1, &textures);
    glBindTexture(GL_TEXTURE_2D, textures);
    glUniform1i(this->shader_tex_pos_, 0);

    /* We require 1 byte alignment when uploading texture data */
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    /* Clamping to edges is important to prevent artifacts when scaling */
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    /* Linear filtering usually looks best for text */
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    /* Set up the VBO for our vertex data */
    glEnableVertexAttribArray(this->shader_coord_pos_);

    for (const auto &text : texts_) {

        if (!text) {
            std::cerr << "Text pointer not valid!" << std::endl;
            continue;
        }

        /* Set font size to 48 pixels, color to black */
        FT_Set_Pixel_Sizes(this->freetype_face_, 0, text->getSize());
        glUniform4fv(this->shader_color_pos_, 1, text->getColor());

        glBindBuffer(GL_ARRAY_BUFFER, text->getVBO());
        glVertexAttribPointer(this->shader_coord_pos_, 4, GL_FLOAT, GL_FALSE, 0, 0);

        auto x_tmp = text->getX();
        auto y_tmp = text->getY();

        float sx = 2.0f / camera->getWindowSize()[0];
        float sy = 2.0f / camera->getWindowSize()[1];

        /* Loop through all characters */
        for (char i : text->getText()) {
            /* Try to load and render the character */
            if (FT_Load_Char(this->freetype_face_, i, FT_LOAD_RENDER)) continue;

            /* Upload the "bitmap", which contains an 8-bit grayscale image, as an alpha texture */
            glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, g->bitmap.width, g->bitmap.rows, 0, GL_ALPHA, GL_UNSIGNED_BYTE, g->bitmap.buffer);

            /* Calculate the vertex and texture coordinates */
            float x2 = x_tmp + g->bitmap_left * sx;
            float y2 = -y_tmp - g->bitmap_top * sy;
            float w  = g->bitmap.width * sx;
            float h  = g->bitmap.rows * sy;

            std::array<GLfloat[4], 4> box {{
                 {    x2,     -y2, 0, 0},
                 {x2 + w,     -y2, 1, 0},
                 {    x2, -y2 - h, 0, 1},
                 {x2 + w, -y2 - h, 1, 1},
            }};

            auto buffer_size = sizeof(GLfloat) * box.size() * 4;

            /* Draw the character on the screen */
            glBufferData(GL_ARRAY_BUFFER, buffer_size, box.data(), GL_DYNAMIC_DRAW);
            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

            /* Advance the cursor to the start of the next character */
            x_tmp += (g->advance.x >> 6) * sx;
            y_tmp += (g->advance.y >> 6) * sy;
        }
    }

    glDisableVertexAttribArray(this->shader_coord_pos_);
    glDeleteTextures(1, &textures);
}