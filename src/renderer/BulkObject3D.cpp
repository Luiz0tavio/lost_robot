//
// Created by luiz0tavio on 6/2/18.
//

#include "BulkObject3D.hpp"

Renderer::BulkObject3D &Renderer::BulkObject3D::getInstance()
{
    static Renderer::BulkObject3D instance;
    return instance;
}

void * Renderer::BulkObject3D::operator new (std::size_t size)
{
    return Memory::Provider::getMemory(Memory::PoolType::POOL_TYPE_GENERIC, size);
}

Renderer::BulkObject3D::BulkObject3D() : objects3d_({}), shader_(nullptr)
{
    // Compile/Link/Set Shader Program
    this->shader_ = new Renderer::Shader();
    this->shader_->createGraphicShader(GL_VERTEX_SHADER, "phong.vert");
    this->shader_->createGraphicShader(GL_FRAGMENT_SHADER, "phong.frag");
    this->shader_->link();

    auto loc = glGetAttribLocation(this->shader_->getShaderProgram(), "vertInPos");
    if (loc < 0) std::cerr << "Error find location 'inPos' Attribute shader!" << std::endl;
    this->shader_vert_pos_ = static_cast<GLuint>(loc);

    loc = glGetAttribLocation(this->shader_->getShaderProgram(), "vertInNormal");
    if (loc < 0) std::cerr << "Error find location 'inNormal' Attribute shader!" << std::endl;
    this->shader_normal_pos_ = static_cast<GLuint>(loc);

    loc = glGetAttribLocation(this->shader_->getShaderProgram(), "vertInUV");
    if (loc < 0) std::cerr << "Error find location 'inUV' Attribute shader!" << std::endl;
    this->shader_uv_pos_ = static_cast<GLuint>(loc);

    loc = glGetUniformLocation(this->shader_->getShaderProgram(), "inTexture");
    if (loc < 0) std::cerr << "Can't find 'inTexture' uniform on shader!" << std::endl;
    this->shader_tex_pos_ = static_cast<GLuint>(loc);
}

void Renderer::BulkObject3D::push_back(Object3D* Object3D)
{
    this->objects3d_.push_back(Object3D);
}

void Renderer::BulkObject3D::remove(Renderer::Object3D *Object3D)
{
    auto &v = this->objects3d_;
    v.erase(std::remove(v.begin(), v.end(), Object3D), v.end());
}

void Renderer::BulkObject3D::draw(Renderer::Camera *camera)
{
    this->shader_->use();

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glEnableVertexAttribArray(this->shader_vert_pos_);
    glEnableVertexAttribArray(this->shader_uv_pos_);

    for (const auto &object3D : objects3d_) {

        camera->update(object3D);

        GLsizei stride = 8 * sizeof(GLfloat);
        glBindBuffer(GL_ARRAY_BUFFER, object3D->getVBO());
        glVertexAttribPointer(this->shader_vert_pos_,   3, GL_FLOAT, GL_TRUE, stride, nullptr);
        glVertexAttribPointer(this->shader_uv_pos_,     2, GL_FLOAT, GL_TRUE, stride, (const GLvoid*)(3 * sizeof(GLfloat)));
        glVertexAttribPointer(this->shader_normal_pos_, 3, GL_FLOAT, GL_TRUE, stride, (const GLvoid*)(5 * sizeof(GLfloat)));

        auto meshes = object3D->getMeshes();
        auto textures = object3D->getTextures();

        for (auto mesh : meshes) {

            std::vector<GLfloat> vertices = {};
            for (auto vertex : mesh->vertices) {
                for (auto vertex_element : vertex) {
                    vertices.push_back(vertex_element);
                }
            }

            if (textures.size() > mesh->texture_index) {
                glActiveTexture(GL_TEXTURE0 + textures[mesh->texture_index]);
                glBindTexture(GL_TEXTURE_2D, textures[mesh->texture_index]);
                glUniform1i(this->shader_tex_pos_, textures[mesh->texture_index]);
            }

            glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(GLfloat), vertices.data(), GL_STATIC_DRAW);
            glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(vertices.size()));
        }
    }

}

