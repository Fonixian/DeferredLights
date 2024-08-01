#pragma once
#include <GL/glew.h>
#include <glm/glm.hpp>
#include <array>
#include <vector>

class Entity;

class EnvironmentMap {
	static GLuint shaderID;

	GLuint frameBuffer = 0;
	GLuint m_CubeMap = 0;
	GLuint m_CubeMapDepth = 0;
	std::array<glm::mat4, 6> transforms;
	float refreshTime;

	void ClearTexture(GLint, GLint);
public:
	GLint resolution;
	float frequency;

	EnvironmentMap(glm::vec3, float);
	~EnvironmentMap();
	void UpdateScene(const std::vector<Entity>&);
	void createFrameBuffer(GLint);
	void updatePosition(glm::vec3, float);
	GLuint getTexture() const;
};