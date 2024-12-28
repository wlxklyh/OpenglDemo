#ifndef SHADER_H
#define SHADER_H

#include <glad/glad.h>
#include <glm/glm.hpp>

#include <iostream>
#include <sstream>
#include <string>
#include <vector>

enum ShaderType { Vertex = 0, Fragment = 1, Geometry = 2 };

class HShader
{
public:
    unsigned int Id;
    std::string VShaderCode;
    std::string FShaderCode;
    std::vector<std::string> Macros;

    template <typename T1>
    void ModifyOrAddMacro(std::string Key, T1 Value);

    HShader(const char* VertexPath, const char* FragmentPath);
    std::string GetFullShaderCode(ShaderType Type);
    void Compile();
    void Use();

    void SetBool(const std::string& Name, bool Value) const;
    void SetInt(const std::string& Name, int Value) const;
    void SetFloat(const std::string& Name, float Value) const;
    void SetVec2(const std::string& Name, const glm::vec2& Value) const;
    void SetVec2(const std::string& Name, float X, float Y) const;
    void SetVec3(const std::string& Name, const glm::vec3& Value) const;
    void SetVec3(const std::string& Name, float x, float y, float z) const;
    void SetVec4(const std::string& Name, const glm::vec4& value) const;
    void SetVec4(const std::string& Name, float x, float y, float z,
                 float w) const;
    void SetMat2(const std::string& Name, const glm::mat2& Mat) const;
    void SetMat3(const std::string& Name, const glm::mat3& Mat) const;
    void SetMat4(const std::string& Name, const glm::mat4& Mat) const;

private:
    void CheckCompileErrors(GLuint Shader, std::string Type,
                            std::string SourceCode = "");
};

template <typename T1>
void HShader::ModifyOrAddMacro(const std::string Key, T1 Value)
{
    std::stringstream ss;
    ss << "#define " << Key << " " << Value << "\t\n";
    
    for (int i = 0; i < Macros.size(); i++)
    {
        if (Macros[i].find(Key) != std::string::npos)
        {
            Macros[i] = ss.str();
            return;
        }
    }

    Macros.push_back(ss.str());
}


#endif
