#include "Entity.h"
#include <glm/gtc/matrix_transform.hpp>

GLuint Entity::LastTextureID = 0;
GLuint Entity::LastProgramID = 0;

void Entity::Reset() {
    LastProgramID = 0;
}

Entity::Entity(const Mesh* mesh, const GLuint texture, const glm::vec3& position = { 0.0f, 0.0f, 0.0f }, const glm::vec3& rotation = { 0.0f, 0.0f, 0.0f }, const glm::vec3& scale = { 1.0f, 1.0f, 1.0f }) :
	mesh(mesh),
	textureID(texture),
	position(position),
	rotation(rotation),
	scale(scale) {}

glm::mat4 Entity::GetLocalModelMatrix() const {
    const glm::mat4 transformX = glm::rotate(glm::mat4(1.0f),
        glm::radians(rotation.x),
        glm::vec3(1.0f, 0.0f, 0.0f));
    const glm::mat4 transformY = glm::rotate(glm::mat4(1.0f),
        glm::radians(rotation.y),
        glm::vec3(0.0f, 1.0f, 0.0f));
    const glm::mat4 transformZ = glm::rotate(glm::mat4(1.0f),
        glm::radians(rotation.z),
        glm::vec3(0.0f, 0.0f, 1.0f));

    const glm::mat4 rotationMatrix = transformY * transformX * transformZ;

    return glm::translate(glm::mat4(1.0f), position) *
        rotationMatrix *
        glm::scale(glm::mat4(1.0f), scale);
}

void Entity::SetGenerateReflection(bool generateReflection) {
    if (generateReflection) {
        environmentMap = std::make_unique<EnvironmentMap>(mesh->center + position, 1);
    } else {
        environmentMap.reset();
    }
};

bool Entity::GetGenerateReflection() const {
    return environmentMap.get();
};

void Entity::SetTexture(GLuint program) const {
    if (program != LastProgramID) {
        LastProgramID = program;
        LastTextureID = 0;
    }

    if (LastTextureID != textureID) {
        LastTextureID = textureID;
        glBindTextureUnit(0, textureID);
    }
}

void Entity::Update(const std::vector<Entity>& entities) {
    if (GetGenerateReflection()) {
        bool lastReflected = reflected;
        reflected = false;
        environmentMap->UpdateScene(entities);
        reflected = lastReflected;
    }
}