#include "EnvironmentMap.h"
#include "Entity.h"
#include "Logs.h"
#include "ProgramBuilder.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <algorithm>

GLuint EnvironmentMap::shaderID = 0;

std::array<glm::mat4, 6> getTransform(const glm::vec3& center, const float& radius) {
	constexpr float aspect = 1.f;

	std::array<glm::mat4, 6> transforms;
	glm::mat4 perspective = glm::perspective(glm::radians(90.0f), aspect, std::min(0.5f, radius), 1000.f);

	transforms[0] = perspective * glm::lookAt(center, center + glm::vec3(1.0, 0.0, 0.0), glm::vec3(0.0, -1.0, 0.0));
	transforms[1] = perspective * glm::lookAt(center, center + glm::vec3(-1.0, 0.0, 0.0), glm::vec3(0.0, -1.0, 0.0));
	transforms[2] = perspective * glm::lookAt(center, center + glm::vec3(0.0, 1.0, 0.0), glm::vec3(0.0, 0.0, 1.0));
	transforms[3] = perspective * glm::lookAt(center, center + glm::vec3(0.0, -1.0, 0.0), glm::vec3(0.0, 0.0, -1.0));
	transforms[4] = perspective * glm::lookAt(center, center + glm::vec3(0.0, 0.0, 1.0), glm::vec3(0.0, -1.0, 0.0));
	transforms[5] = perspective * glm::lookAt(center, center + glm::vec3(0.0, 0.0, -1.0), glm::vec3(0.0, -1.0, 0.0));

	return transforms;
}

EnvironmentMap::EnvironmentMap(glm::vec3 center, float radius) : resolution(480), frequency(10.f), refreshTime(0.0f) {
	if (!shaderID) {
		shaderID = glCreateProgram();
		ProgramBuilder{ shaderID }
			.ShaderStage(GL_VERTEX_SHADER, "Shaders/environment.vert")
			.ShaderStage(GL_GEOMETRY_SHADER, "Shaders/environment.geom")
			.ShaderStage(GL_FRAGMENT_SHADER, "Shaders/environment.frag")
			.Link();
		std::atexit([]() {
			glDeleteProgram(shaderID);
		});
	}
	transforms = getTransform(center, radius);
	createFrameBuffer(resolution);
}

EnvironmentMap::~EnvironmentMap() {
	glDeleteTextures(1, &m_CubeMap);
	glDeleteTextures(1, &m_CubeMapDepth);
	glDeleteFramebuffers(1, &frameBuffer);
}

void EnvironmentMap::ClearTexture(GLint from, GLint to) {
	if (to == 0) return;
	static glm::vec3 clearValue = glm::vec3(0.0f);
	static float clearValueDepth = 1.0f;

	glClearTexSubImage(m_CubeMap, 0, 0, 0, from, resolution, resolution,
		to - from, GL_RGB, GL_FLOAT, &clearValue);

	glClearTexSubImage(m_CubeMapDepth, 0, 0, 0, from, resolution, resolution,
		to - from, GL_DEPTH_COMPONENT, GL_FLOAT, &clearValueDepth);
}

void EnvironmentMap::UpdateScene(const std::vector<Entity>& entities) {
	bool update = (int)(refreshTime + 6.0f / frequency) > (int)refreshTime;
	int from = (int)refreshTime;
	refreshTime = std::fmod(refreshTime + 6.0f / frequency, 6.0f);
	int to = (int)refreshTime;
	if (!update) return;

	std::array<int, 6> updateValues;
	updateValues.fill(0);

	if (from < to) {
		std::fill_n(updateValues.begin() + from, to - from, 1);
		ClearTexture(from, to);
	} else if (from == to) {
		updateValues.fill(1);
		ClearTexture(0, 6);
	} else {
		std::fill_n(updateValues.begin() + from, 6 - from, 1);
		std::fill_n(updateValues.begin(), to, 1);
		ClearTexture(from, 6);
		if (to > 0) ClearTexture(0, to);
	}

	glViewport(0, 0, resolution, resolution);
	glBindFramebuffer(GL_FRAMEBUFFER, frameBuffer);
	glUseProgram(shaderID);

	glUniformMatrix4fv(1, 6, GL_FALSE, (float*)transforms.data());
	glUniform1iv(7, 6, updateValues.data());
	for (const Entity& entity : entities) {
		if (entity.reflected) {
			entity.SetTexture(shaderID);
			glUniformMatrix4fv(0, 1, GL_FALSE, glm::value_ptr(entity.GetLocalModelMatrix()));
			glBindVertexArray(entity.mesh->mesh.vaoID);
			glDrawElements(GL_TRIANGLES, entity.mesh->mesh.count, GL_UNSIGNED_INT, nullptr);
		}
	}
	
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void EnvironmentMap::createFrameBuffer(GLint resolution) {
	if (frameBuffer) {
		glDeleteTextures(1, &m_CubeMap);
		glDeleteTextures(1, &m_CubeMapDepth);
		glDeleteFramebuffers(1, &frameBuffer);
	}

	glCreateTextures(GL_TEXTURE_CUBE_MAP, 1, &m_CubeMap);
	glTextureStorage2D(m_CubeMap, 1, GL_RGB8, resolution, resolution);

	glTextureParameteri(m_CubeMap, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTextureParameteri(m_CubeMap, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTextureParameteri(m_CubeMap, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTextureParameteri(m_CubeMap, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTextureParameteri(m_CubeMap, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

	glCreateFramebuffers(1, &frameBuffer);
	glNamedFramebufferTexture(frameBuffer, GL_COLOR_ATTACHMENT0, m_CubeMap, 0);
	CheckGlError("Error creating depth attachment");

	glCreateTextures(GL_TEXTURE_CUBE_MAP, 1, &m_CubeMapDepth);
	glTextureStorage2D(m_CubeMapDepth, 1, GL_DEPTH_COMPONENT24, resolution, resolution);

	glTextureParameteri(m_CubeMapDepth, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTextureParameteri(m_CubeMapDepth, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTextureParameteri(m_CubeMapDepth, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTextureParameteri(m_CubeMapDepth, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTextureParameteri(m_CubeMapDepth, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

	glNamedFramebufferTexture(frameBuffer, GL_DEPTH_ATTACHMENT, m_CubeMapDepth, 0);
	CheckGlError("Error creating depth attachment");

	CheckFramebufferError(frameBuffer);
}

void EnvironmentMap::updatePosition(glm::vec3 center, float radius) {
	transforms = getTransform(center, radius);
}

GLuint EnvironmentMap::getTexture() const {
	return m_CubeMap;
}