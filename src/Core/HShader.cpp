#include "HShader.h"

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>



HShader::HShader(const char* VertexPath, const char* FragmentPath): HasCompiled(false)
{
    // 1. retrieve the vertex/fragment source code from filePath
    std::string vertexCode;
    std::string fragmentCode;
    std::ifstream vShaderFile;
    std::ifstream fShaderFile;
    // ensure ifstream objects can throw exceptions:
    vShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    fShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    try
    {
        // open files
        vShaderFile.open(VertexPath);
        fShaderFile.open(FragmentPath);
        std::stringstream vShaderStream, fShaderStream;
        // read file's buffer contents into streams
        vShaderStream << vShaderFile.rdbuf();
        fShaderStream << fShaderFile.rdbuf();
        // close file handlers
        vShaderFile.close();
        fShaderFile.close();
        // convert stream into string
        vertexCode = vShaderStream.str();
        fragmentCode = fShaderStream.str();
    }
    catch (std::ifstream::failure& e)
    {
        std::cout << "ERROR::SHADER::FILE_NOT_SUCCESFULLY_READ" << std::endl;
    }
    VShaderCode = vertexCode;
    FShaderCode = fragmentCode;
}

std::string HShader::GetFullShaderCode(ShaderType Type)
{
    std::string FullCode = "#version 430\n";
    for (int i = 0; i < Macros.size(); i++)
    {
        FullCode += Macros[i];
    }
    switch (Type)
    {
    case Vertex:
        FullCode = FullCode + VShaderCode;
        break;
    case Fragment:
        FullCode = FullCode + FShaderCode;
        break;
    }
    return FullCode;
}

void HShader::Compile()
{
    if (HasCompiled)
    {
        return;
    }
    unsigned int vertex, fragment;
    // vertex shader
    vertex = glCreateShader(GL_VERTEX_SHADER);
    std::string StrVSShaderCode = GetFullShaderCode(Vertex);
    const char* vsShaderCode = StrVSShaderCode.c_str();
    glShaderSource(vertex, 1, &vsShaderCode, nullptr);
    glCompileShader(vertex);
    CheckCompileErrors(vertex, "VERTEX", StrVSShaderCode);

    // fragment Shader
    fragment = glCreateShader(GL_FRAGMENT_SHADER);
    std::string StrFSShaderCode = GetFullShaderCode(Fragment);
    const char* fsShaderCode = StrFSShaderCode.c_str();
    glShaderSource(fragment, 1, &fsShaderCode, nullptr);
    glCompileShader(fragment);
    CheckCompileErrors(fragment, "FRAGMENT", StrFSShaderCode);

    // shader Program
    Id = glCreateProgram();
    glAttachShader(Id, vertex);
    glAttachShader(Id, fragment);
    glLinkProgram(Id);
    CheckCompileErrors(Id, "PROGRAM");
    // delete the shaders as they're linked into our program now and no longer
    // necessery
    glDeleteShader(vertex);
    glDeleteShader(fragment);
    HasCompiled = true;
}

void HShader::Use()
{
    glUseProgram(Id);
}

void HShader::SetBool(const std::string& Name, bool Value) const
{
    glUniform1i(glGetUniformLocation(Id, Name.c_str()), static_cast<int>(Value));
}

void HShader::SetInt(const std::string& Name, int Value) const
{
    glUniform1i(glGetUniformLocation(Id, Name.c_str()), Value);
}

void HShader::SetFloat(const std::string& Name, float Value) const
{
    glUniform1f(glGetUniformLocation(Id, Name.c_str()), Value);
}

void HShader::SetVec2(const std::string& Name, const glm::vec2& Value) const
{
    glUniform2fv(glGetUniformLocation(Id, Name.c_str()), 1, &Value[0]);
}

void HShader::SetVec2(const std::string& Name, float X, float Y) const
{
    glUniform2f(glGetUniformLocation(Id, Name.c_str()), X, Y);
}

void HShader::SetVec3(const std::string& Name, const glm::vec3& Value) const
{
    glUniform3fv(glGetUniformLocation(Id, Name.c_str()), 1, &Value[0]);
}

void HShader::SetVec3(const std::string& Name, float x, float y,
                      float z) const
{
    glUniform3f(glGetUniformLocation(Id, Name.c_str()), x, y, z);
}

void HShader::SetVec4(const std::string& Name, const glm::vec4& value) const
{
    glUniform4fv(glGetUniformLocation(Id, Name.c_str()), 1, &value[0]);
}

void HShader::SetVec4(const std::string& Name, float x, float y, float z,
                      float w) const
{
    glUniform4f(glGetUniformLocation(Id, Name.c_str()), x, y, z, w);
}

void HShader::SetMat2(const std::string& Name, const glm::mat2& Mat) const
{
    glUniformMatrix2fv(glGetUniformLocation(Id, Name.c_str()), 1, GL_FALSE,
                       &Mat[0][0]);
}

void HShader::SetMat3(const std::string& Name, const glm::mat3& Mat) const
{
    glUniformMatrix3fv(glGetUniformLocation(Id, Name.c_str()), 1, GL_FALSE,
                       &Mat[0][0]);
}

void HShader::SetMat4(const std::string& Name, const glm::mat4& Mat) const
{
    glUniformMatrix4fv(glGetUniformLocation(Id, Name.c_str()), 1, GL_FALSE,
                       &Mat[0][0]);
}

void HShader::CheckCompileErrors(GLuint Shader, std::string Type,
                                 std::string Sourcecode)
{
    GLint success;
    GLchar infoLog[1024];
    if (Type != "PROGRAM")
    {
        glGetShaderiv(Shader, GL_COMPILE_STATUS, &success);
        if (!success)
        {
            glGetShaderInfoLog(Shader, 1024, nullptr, infoLog);
            std::cout
                << "ERROR::SHADER_COMPILATION_ERROR of type: " << Type << "\n"
                << infoLog
                << "\n -- --------------------------------------------------- -- "
                << std::endl;
        }
    }
    else
    {
        glGetProgramiv(Shader, GL_LINK_STATUS, &success);
        if (!success)
        {
            glGetProgramInfoLog(Shader, 1024, nullptr, infoLog);
            std::cout
                << "ERROR::PROGRAM_LINKING_ERROR of type: " << Type << "\n"
                << infoLog
                << "\n -- --------------------------------------------------- -- "
                << std::endl;
        }
    }
    if (!success && !Sourcecode.empty())
    {
        std::cout << "Shader source code: " << Sourcecode << std::endl;
    }
}
