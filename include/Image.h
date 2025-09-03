#ifndef IMAGE_H
#define IMAGE_H

#include <string>
#include <stdexcept>
#include <glad/glad.h>  // or <GL/glew.h> depending on your setup
#include "stb_image.h"

class Image
{
public:
    // Constructor loads image and creates OpenGL texture
    Image(const std::string& path, int desiredChannels = 4)
        : w_(0), h_(0), c_(0), data_(nullptr), textureID_(0)
    {
        loadFromFile(path, desiredChannels);
        createGLTexture();
    }

    // Deleted default constructor
    Image() = delete;

    // Move constructor
    Image(Image&& other) noexcept
        : w_(other.w_), h_(other.h_), c_(other.c_),
          data_(other.data_), textureID_(other.textureID_)
    {
        other.data_ = nullptr;
        other.textureID_ = 0;
        other.w_ = other.h_ = other.c_ = 0;
    }

    // Move assignment
    Image& operator=(Image&& other) noexcept
    {
        if (this != &other)
        {
            free();
            w_ = other.w_;
            h_ = other.h_;
            c_ = other.c_;
            data_ = other.data_;
            textureID_ = other.textureID_;

            other.data_ = nullptr;
            other.textureID_ = 0;
            other.w_ = other.h_ = other.c_ = 0;
        }
        return *this;
    }

    // Deleted copy semantics
    Image(const Image&) = delete;
    Image& operator=(const Image&) = delete;

    ~Image()
    {
        free();
    }

    int width() const { return w_; }
    int height() const { return h_; }
    int channels() const { return c_; }
    const unsigned char* data() const { return data_; }
    GLuint textureID() const { return textureID_; }

private:
    void loadFromFile(const std::string& path, int desiredChannels)
    {
        FILE* img = fopen(path.c_str(), "rb");
        if (!img)
            throw std::runtime_error("Image file not found: " + path);

        stbi_set_flip_vertically_on_load(true);
        data_ = stbi_load_from_file(img, &w_, &h_, &c_, desiredChannels);
        fclose(img);

        if (!data_)
            throw std::runtime_error("Failed to load image: " + path);

        if (desiredChannels > 0)
            c_ = desiredChannels;
    }

    void createGLTexture()
    {
        if (!data_) return;

        glGenTextures(1, &textureID_);
        glBindTexture(GL_TEXTURE_2D, textureID_);

        GLenum format = GL_RGBA;
        if (c_ == 1) format = GL_RED;
        else if (c_ == 3) format = GL_RGB;
        else if (c_ == 4) format = GL_RGBA;

        glTexImage2D(GL_TEXTURE_2D, 0, format, w_, h_, 0, format, GL_UNSIGNED_BYTE, data_);
        glGenerateMipmap(GL_TEXTURE_2D);

        // Default texture parameters
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        glBindTexture(GL_TEXTURE_2D, 0);
    }

    void free()
    {
        if (data_)
        {
            stbi_image_free(data_);
            data_ = nullptr;
        }
        if (textureID_ != 0)
        {
            glDeleteTextures(1, &textureID_);
            textureID_ = 0;
        }
        w_ = h_ = c_ = 0;
    }

private:
    int w_, h_, c_;
    unsigned char* data_;
    GLuint textureID_;
};

#endif // IMAGE_H
