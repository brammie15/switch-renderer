#ifndef SHADER_H
#define SHADER_H

#include <string>
#include <glad/glad.h>

class Shader
{
public:
    Shader() = default;
    ~Shader();

    // Load, compile and link from two GLSL source files. Paths are treated
    // like normal file system paths (e.g. romfs:/shaders/vertex.glsl).
    bool loadFromFiles(const std::string& vertPath, const std::string& fragPath);

    void use() const { glUseProgram(program_); }
    GLuint program() const { return program_; }

    // convenience wrapper
    GLint getUniformLocation(const char* name) const { return glGetUniformLocation(program_, name); }

private:
    std::string readFile(const std::string& path) const;
    bool compileShader(GLenum type, const char* source, GLuint& outShader) const;

    GLuint program_{0};
};

#endif // SHADER_H