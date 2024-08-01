#pragma once
#include <GL/glew.h>
#include <glm/glm.hpp>
#include <array>
#include <vector>
#include "Camera.h"
#include "Entity.h"

template<int>
class DirLightShadow;

class PointLightShadow;

enum LightType {
	POINT_LIGHT = 0,
	DIRECTIONAL_LIGHT = 1,
	POINT_SHADOWED_LIGHT = 2,
	DIRECTIONAL_SHADOWED_LIGHT = 3
};

struct LightInfo {
	glm::vec3 color = glm::vec3(0, 0, 0);
	union {
		glm::vec4 position = glm::vec4(0, 0, 0, 1);
		glm::vec4 direction;
	};
	std::array<int, 2> shadowMapResolutionWH = { 1024, 1024 };
	float refreshFrequency = 1.0f;
	bool castShadow = false;

	LightInfo(const glm::vec3&, const glm::vec4&, const bool, const std::array<int,2>&);
	LightInfo() = default;
};

class LightBuffer {
	std::vector<LightInfo> m_LightInfos;
	GLuint m_Buffer = 0;
public:
	LightBuffer();
	~LightBuffer();

	void Bind(GLuint) const;
	// Doesn't happen often, so the implementation can be less then ideal
	void AddLight(const LightInfo&);
	void UpdateLight(size_t);
	void DeleteLight(size_t);

	std::vector<LightInfo>& GetInfos();
	const std::vector<LightInfo>& GetInfos() const;
	GLsizei GetSize() const;
};

class Lights {
	GLuint m_FrameBufferID = 0;
	GLuint m_TextureID = 0;

	std::array<GLuint, 4> m_LightShaderIDs = {};
	GLuint m_PointShadowShaderID = 0;
	GLuint m_DirectionalShadowShaderID = 0;

	std::array<LightBuffer, 4> m_LightBuffers;
	std::vector<DirLightShadow<5>> dirShadows;
	std::vector<PointLightShadow> pointShadows;

	std::array<float, 4> shadowCascadeLevels;

	void renderPointLights(GLuint, GLuint, GLuint, const Camera&, LightType) const;
	void renderDirectionalLights(GLuint, GLuint, const Camera&, LightType) const;
	void renderPointLightsShadowed(GLuint, GLuint, GLuint, const Camera&) const;
	void renderDirectionalLightsShadowed(GLuint, GLuint, GLuint, const Camera&) const;
	
	std::vector<glm::vec4> getFrustumCornersWorldSpace(const glm::mat4&) const;
	glm::mat4 getLightSpaceMatrix(const float, const float, const glm::vec3&, const Camera&) const;
	std::vector<glm::mat4> getLightSpaceMatrices(const glm::vec3&, const Camera&) const;
public:
	Lights();
	~Lights();

	void RenderLights(GLuint, GLuint, GLuint, const Camera&) const;
	void UpdateShadowMaps(const std::vector<Entity>&, const Camera&);

	void CreateFrameBuffer(GLint, GLint, GLuint);
	GLuint GetLightTexture() const;
	std::vector<LightInfo>& GetInfo(LightType);

	void AddLight(LightType, const LightInfo& = {});
	void UpdateLight(LightType, size_t);
	void DeleteLight(LightType, size_t);

	void ChangeShadowed(LightType, size_t);
};