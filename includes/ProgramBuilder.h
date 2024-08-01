#pragma once

#include <filesystem>
#include <GL/glew.h>
#include <vector>

class ProgramBuilder
{
private:
	const GLuint programID;
	std::vector<GLuint> shaderIDs{};
protected:
	void LoadShader(const GLuint, const std::filesystem::path&);
	void CompileShaderFromSource(const GLuint, std::string_view);
	void AttachShader(const GLuint, const std::filesystem::path&);
public:
	ProgramBuilder(GLuint);
	~ProgramBuilder();
	ProgramBuilder& ShaderStage(const GLuint, const std::filesystem::path&);
	void Link();
};