#pragma once
#include <glm/glm.hpp>
#include <GL/glew.h>
#include <memory>
#include "GLUtils.hpp"
#include "EnvironmentMap.h"
#include "Mesh.h"

class Entity {
	static GLuint LastTextureID;
	static GLuint LastProgramID;
public:
	static void Reset();

	const Mesh* mesh;
	const GLuint textureID;
	
	glm::vec3 position;
	glm::vec3 rotation;
	glm::vec3 scale;

	bool castShadow = true;
	bool receiveShadow = true;
	bool reflected = true;

	std::unique_ptr<EnvironmentMap> environmentMap;

	Entity(const Mesh*, const GLuint, const glm::vec3&, const glm::vec3&, const glm::vec3&);
	void SetGenerateReflection(bool);
	bool GetGenerateReflection() const;

	glm::mat4 GetLocalModelMatrix() const;
	void Update(const std::vector<Entity>&);
	void SetTexture(GLuint) const;
	void Moved() {};
};