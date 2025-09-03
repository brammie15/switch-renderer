#include "Shader.h"
#include <fstream>
#include <sstream>
#include <cstdio>

static std::string readFileImpl(const std::string& path)
{
    std::ifstream ifs(path, std::ios::in | std::ios::binary);
    if (!ifs)
        return {};
    std::ostringstream ss;
    ss << ifs.rdbuf();
    return ss.str();
}

Shader::~Shader()
{
    if (program_)
    {
        glDeleteProgram(program_);
        program_ = 0;
    }
}

std::string Shader::readFile(const std::string& path) const
{
    return readFileImpl(path);
}

bool Shader::compileShader(GLenum type, const char* source, GLuint& outShader) const
{
    GLint success;
    GLchar info[1024];
    GLuint sh = glCreateShader(type);
    if (!sh)
    {
        printf("glCreateShader failed\n");
        return false;
    }
    glShaderSource(sh, 1, &source, nullptr);
    glCompileShader(sh);
    glGetShaderiv(sh, GL_COMPILE_STATUS, &success);
    if (success == GL_FALSE)
    {
        glGetShaderInfoLog(sh, sizeof(info), nullptr, info);
        printf("Shader compile error: %s\n", info);
        glDeleteShader(sh);
        return false;
    }
    outShader = sh;
    return true;
}

bool Shader::loadFromFiles(const std::string& vertPath, const std::string& fragPath)
{
    std::string vertSrc = readFile(vertPath);
    if (vertSrc.empty())
    {
        printf("Failed to read vertex shader: %s\n", vertPath.c_str());
        return false;
    }
    std::string fragSrc = readFile(fragPath);
    if (fragSrc.empty())
    {
        printf("Failed to read fragment shader: %s\n", fragPath.c_str());
        return false;
    }

    GLuint vsh = 0, fsh = 0;
    if (!compileShader(GL_VERTEX_SHADER, vertSrc.c_str(), vsh))
        return false;
    if (!compileShader(GL_FRAGMENT_SHADER, fragSrc.c_str(), fsh))
    {
        glDeleteShader(vsh);
        return false;
    }

    program_ = glCreateProgram();
    glAttachShader(program_, vsh);
    glAttachShader(program_, fsh);
    glLinkProgram(program_);

    GLint success = GL_FALSE;
    glGetProgramiv(program_, GL_LINK_STATUS, &success);
    if (success == GL_FALSE)
    {
        char buf[1024];
        glGetProgramInfoLog(program_, sizeof(buf), nullptr, buf);
        printf("Shader link error: %s\n", buf);
        glDeleteShader(vsh);
        glDeleteShader(fsh);
        glDeleteProgram(program_);
        program_ = 0;
        return false;
    }

    // shaders attached to the program can be deleted after linking
    glDeleteShader(vsh);
    glDeleteShader(fsh);
    return true;
}
