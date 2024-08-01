#include "ProgramBuilder.h"

#include "SDL2/SDL_log.h"
#include <fstream>
#include <string>


ProgramBuilder::ProgramBuilder(const GLuint _programID) : programID(_programID)
{
	if (programID == 0)
	{
		SDL_LogMessage(SDL_LOG_CATEGORY_ERROR,
			SDL_LOG_PRIORITY_ERROR,
			"Program needs to be inited before loading!");
		return;
	}
}

ProgramBuilder::~ProgramBuilder()
{
	if (!shaderIDs.empty())
	{
		SDL_LogMessage(SDL_LOG_CATEGORY_ERROR,
			SDL_LOG_PRIORITY_ERROR,
			"[ProgramBuilder] Program is not linked!");
	}
}

void ProgramBuilder::LoadShader( const GLuint loadedShader, const std::filesystem::path& fileName )
{
	// Upon failure, log the error message and return with -1
	if (loadedShader == 0)
	{
		SDL_LogMessage(SDL_LOG_CATEGORY_ERROR,
			SDL_LOG_PRIORITY_ERROR,
			"Shader needs to be inited before loading %s !", fileName.string().c_str());
		return;
	}

	// Loading a shader from disk
	std::string shaderCode = "";

	// Open 'fileName'
	std::ifstream shaderStream(fileName);
	if (!shaderStream.is_open())
	{
		SDL_LogMessage(SDL_LOG_CATEGORY_ERROR,
			SDL_LOG_PRIORITY_ERROR,
			"Error while opening shader file %s!", fileName.string().c_str());
		return;
	}

	// Load the contents of the file into the 'shaderCode' variable.
	std::string line = "";
	while (std::getline(shaderStream, line))
	{
		shaderCode += line + "\n";
	}

	shaderStream.close();

	CompileShaderFromSource(loadedShader, shaderCode);
}

void ProgramBuilder::CompileShaderFromSource(const GLuint loadedShader, std::string_view shaderCode)
{
	// Assign the loaded source code to the shader object
	const char* sourcePointer = shaderCode.data();
	GLint sourceLength = static_cast<GLint>(shaderCode.length());

	glShaderSource(loadedShader, 1, &sourcePointer, &sourceLength);

	// Let's compile the shader
	glCompileShader(loadedShader);

	// Check whether the compilation was successful
	GLint result = GL_FALSE;
	int infoLogLength;

	// For this, retrieve the status
	glGetShaderiv(loadedShader, GL_COMPILE_STATUS, &result);
	glGetShaderiv(loadedShader, GL_INFO_LOG_LENGTH, &infoLogLength);

	if (GL_FALSE == result || infoLogLength != 0)
	{
		// Get and log the error message.
		std::string ErrorMessage(infoLogLength, '\0');
		glGetShaderInfoLog(loadedShader, infoLogLength, NULL, ErrorMessage.data());

		SDL_LogMessage(SDL_LOG_CATEGORY_ERROR,
			(result) ? SDL_LOG_PRIORITY_WARN : SDL_LOG_PRIORITY_ERROR,
			"[glLinkProgram] Shader compile error: %s", ErrorMessage.data());
	}
}

void ProgramBuilder::AttachShader(const GLuint shaderID, const std::filesystem::path& fileName)
{
	shaderIDs.push_back(shaderID);
	LoadShader(shaderID, fileName);

	// Add shader to the program
	glAttachShader(programID, shaderID);
}

ProgramBuilder& ProgramBuilder::ShaderStage(const GLuint shaderType, const std::filesystem::path& fileName)
{
	GLuint shaderID = glCreateShader(shaderType);
	shaderIDs.push_back(shaderID); // We want to clean up later
	LoadShader(shaderID, fileName);

	// Add shader to the program
	glAttachShader(programID, shaderID);
	return *this;
}

void ProgramBuilder::Link()
{
	// We link the shaders (connecting outgoing-incoming variables etc.)
	glLinkProgram(programID);

	// Check for linking errors
	GLint infoLogLength = 0, result = 0;

	glGetProgramiv(programID, GL_LINK_STATUS, &result);
	glGetProgramiv(programID, GL_INFO_LOG_LENGTH, &infoLogLength);
	if (GL_FALSE == result || infoLogLength != 0)
	{
		std::string ErrorMessage(infoLogLength, '\0');
		glGetProgramInfoLog(programID, infoLogLength, nullptr, ErrorMessage.data());
		SDL_LogMessage(SDL_LOG_CATEGORY_ERROR,
			(result) ? SDL_LOG_PRIORITY_WARN : SDL_LOG_PRIORITY_ERROR,
			"[glLinkProgram] Shader linking error: %s", ErrorMessage.data());
	}

	// No need for these
	std::for_each(shaderIDs.begin(), shaderIDs.end(), glDeleteShader);
	shaderIDs.clear();
}