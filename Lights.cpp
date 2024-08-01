#include "Lights.h"
#include "ProgramBuilder.h"
#include "Shadows.h"
#include "Logs.h"
#include <glm/gtc/type_ptr.hpp>

struct Light {
	glm::vec4 color;
	glm::vec4 positionDirection;

	Light(const LightInfo& lightInfo) : color(lightInfo.color, sqrt((lightInfo.color.r + lightInfo.color.g + lightInfo.color.b) / 0.05)), positionDirection(lightInfo.position) {}
};

LightInfo::LightInfo(const glm::vec3& color, const glm::vec4& position, const bool castShadow, const std::array<int, 2>& resolutionWH) :
	color(color),
	position(position),
	shadowMapResolutionWH(resolutionWH),
	castShadow(castShadow),
	refreshFrequency(1.0f) {}

LightBuffer::LightBuffer() {
	glCreateBuffers(1, &m_Buffer);
}

LightBuffer::~LightBuffer() {
	if(m_Buffer) glDeleteBuffers(1, &m_Buffer);
}

void LightBuffer::Bind(GLuint base) const {
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, base, m_Buffer);
}

void LightBuffer::AddLight(const LightInfo& lightInfo) {
	m_LightInfos.push_back(lightInfo);
	std::vector<Light> lightData;
	lightData.reserve(m_LightInfos.size());
	for (const LightInfo& lightInfo : m_LightInfos) lightData.emplace_back(lightInfo);
	glNamedBufferData(m_Buffer, lightData.size() * sizeof(Light), lightData.data(), GL_STATIC_DRAW);
}

void LightBuffer::UpdateLight(size_t index) {
	Light light = m_LightInfos[index];
	glNamedBufferSubData(m_Buffer, index * sizeof(Light), sizeof(Light), &light);
}

void LightBuffer::DeleteLight(size_t index) {
	m_LightInfos.erase(m_LightInfos.begin() + index);
	std::vector<Light> lightData;
	lightData.reserve(m_LightInfos.size());
	for (const LightInfo& lightInfo : m_LightInfos) lightData.emplace_back(lightInfo);
	glNamedBufferData(m_Buffer, lightData.size() * sizeof(Light), lightData.data(), GL_STATIC_DRAW);
}

std::vector<LightInfo>& LightBuffer::GetInfos() {
	return m_LightInfos;
}

const std::vector<LightInfo>& LightBuffer::GetInfos() const {
	return m_LightInfos;
}

GLsizei LightBuffer::GetSize() const {
	return static_cast<GLsizei>(m_LightInfos.size());
}

Lights::Lights() {
	shadowCascadeLevels = { 1000.f / 50.0f, 1000.f / 25.0f, 1000.f / 10.0f, 1000.f / 2.0f };

	for (GLuint& program : m_LightShaderIDs) program = glCreateProgram();

	ProgramBuilder{ m_LightShaderIDs[POINT_LIGHT] }
		.ShaderStage(GL_VERTEX_SHADER, "Shaders/deferred_point2.vert")
		.ShaderStage(GL_TESS_CONTROL_SHADER, "Shaders/deferred_point.tesc")
		.ShaderStage(GL_TESS_EVALUATION_SHADER, "Shaders/deferred_point.tese")
		.ShaderStage(GL_FRAGMENT_SHADER, "Shaders/deferred_point.frag")
		.Link();

	ProgramBuilder{ m_LightShaderIDs[DIRECTIONAL_LIGHT] }
		.ShaderStage(GL_VERTEX_SHADER, "Shaders/deferred_dir.vert")
		.ShaderStage(GL_FRAGMENT_SHADER, "Shaders/deferred_dir.frag")
		.Link();

	ProgramBuilder{ m_LightShaderIDs[POINT_SHADOWED_LIGHT] }
		.ShaderStage(GL_VERTEX_SHADER, "Shaders/deferred_shadow_point.vert")
		.ShaderStage(GL_TESS_CONTROL_SHADER, "Shaders/deferred_shadow_point.tesc")
		.ShaderStage(GL_TESS_EVALUATION_SHADER, "Shaders/deferred_shadow_point.tese")
		.ShaderStage(GL_FRAGMENT_SHADER, "Shaders/deferred_shadow_point.frag")
		.Link();

	ProgramBuilder{ m_LightShaderIDs[DIRECTIONAL_SHADOWED_LIGHT] }
		.ShaderStage(GL_VERTEX_SHADER, "Shaders/deferred_shadow_dir.vert")
		.ShaderStage(GL_FRAGMENT_SHADER, "Shaders/deferred_shadow_dir.frag")
		.Link();

	m_PointShadowShaderID = glCreateProgram();
	ProgramBuilder{ m_PointShadowShaderID }
		.ShaderStage(GL_VERTEX_SHADER, "Shaders/shadow_point.vert")
		.ShaderStage(GL_GEOMETRY_SHADER, "Shaders/shadow_point.geom")
		.ShaderStage(GL_FRAGMENT_SHADER, "Shaders/shadow_point.frag")
		.Link();

	m_DirectionalShadowShaderID = glCreateProgram();
	ProgramBuilder{ m_DirectionalShadowShaderID }
		.ShaderStage(GL_VERTEX_SHADER, "Shaders/shadow_point.vert")
		.ShaderStage(GL_GEOMETRY_SHADER, "Shaders/shadow_dir.geom")
		.Link();
}

Lights::~Lights() {
	if (m_FrameBufferID) {
		glDeleteTextures(1, &m_TextureID);
		glDeleteFramebuffers(1, &m_FrameBufferID);
	}

	for (GLuint programID : m_LightShaderIDs) glDeleteProgram(programID);
	glDeleteProgram(m_PointShadowShaderID);
	glDeleteProgram(m_DirectionalShadowShaderID);
}

void Lights::RenderLights(GLuint diffuseBuffer, GLuint normalBuffer, GLuint depthBuffer, const Camera& camera) const {
	glBindFramebuffer(GL_FRAMEBUFFER, m_FrameBufferID);
	glClearColor(0, 0, 0, 0);
	glClear(GL_COLOR_BUFFER_BIT);

	glEnable(GL_BLEND);
	glBlendEquation(GL_FUNC_ADD);
	glBlendFunc(GL_ONE, GL_ONE);

	renderPointLights(diffuseBuffer, normalBuffer, depthBuffer, camera, POINT_LIGHT);
	renderDirectionalLights(diffuseBuffer, normalBuffer, camera, DIRECTIONAL_LIGHT);

	glEnable(GL_STENCIL_TEST);
	glStencilFunc(GL_EQUAL, 0, 0xFF);
	glStencilMask(0x00);

	renderPointLightsShadowed(diffuseBuffer, normalBuffer, depthBuffer, camera);
	renderDirectionalLightsShadowed(diffuseBuffer, normalBuffer, depthBuffer, camera);

	glStencilFunc(GL_EQUAL, 1, 0xFF);

	renderPointLights(diffuseBuffer, normalBuffer, depthBuffer, camera, POINT_SHADOWED_LIGHT);
	renderDirectionalLights(diffuseBuffer, normalBuffer, camera, DIRECTIONAL_SHADOWED_LIGHT);

	glDisable(GL_STENCIL_TEST);
	glDisable(GL_BLEND);
}

std::array<glm::mat4, 6> getTransform(const LightInfo& info, const PointLightShadow& shadow, const Camera& camera) {
	constexpr float aspect = 1.f;
	constexpr float near = 0.01f;
	const float far = shadow.GetRadius();

	std::array<glm::mat4, 6> transforms;
	glm::mat4 perspective = glm::perspective(glm::radians(90.0f), aspect, near, far);

	transforms[0] = perspective * glm::lookAt(glm::vec3(info.position), glm::vec3(info.position) + glm::vec3(1.0, 0.0, 0.0), glm::vec3(0.0, -1.0, 0.0));
	transforms[1] = perspective * glm::lookAt(glm::vec3(info.position), glm::vec3(info.position) + glm::vec3(-1.0, 0.0, 0.0), glm::vec3(0.0, -1.0, 0.0));
	transforms[2] = perspective * glm::lookAt(glm::vec3(info.position), glm::vec3(info.position) + glm::vec3(0.0, 1.0, 0.0), glm::vec3(0.0, 0.0, 1.0));
	transforms[3] = perspective * glm::lookAt(glm::vec3(info.position), glm::vec3(info.position) + glm::vec3(0.0, -1.0, 0.0), glm::vec3(0.0, 0.0, -1.0));
	transforms[4] = perspective * glm::lookAt(glm::vec3(info.position), glm::vec3(info.position) + glm::vec3(0.0, 0.0, 1.0), glm::vec3(0.0, -1.0, 0.0));
	transforms[5] = perspective * glm::lookAt(glm::vec3(info.position), glm::vec3(info.position) + glm::vec3(0.0, 0.0, -1.0), glm::vec3(0.0, -1.0, 0.0));

	return transforms;
}

void Lights::UpdateShadowMaps(const std::vector<Entity>& entities, const Camera& camera) {
	int windowValues[4];
	glGetIntegerv(GL_VIEWPORT, windowValues);
	//Point Shadow
	glUseProgram(m_PointShadowShaderID);
	const std::vector<LightInfo>& infos = m_LightBuffers[POINT_SHADOWED_LIGHT].GetInfos();
	for (size_t i = 0; i < infos.size(); ++i) {
		std::array<int, 6> update;
		if (!pointShadows[i].Update(infos[i].refreshFrequency, update, infos[i])) continue;
		pointShadows[i].Bind(infos[i]);

		std::array<glm::mat4, 6> transforms = getTransform(infos[i], pointShadows[i], camera);

		glUniformMatrix4fv(3, 6, GL_FALSE, (float*)transforms.data());
		glUniform1iv(9, 6, update.data());
		glUniform3fv(1, 1, glm::value_ptr(infos[i].position));
		glUniform1f(2, pointShadows[i].GetRadius());

		for (const auto& entity : entities) {
			if (entity.castShadow) {
				glUniformMatrix4fv(0, 1, GL_FALSE, glm::value_ptr(entity.GetLocalModelMatrix()));
				glBindVertexArray(entity.mesh->mesh.vaoID);
				glDrawElements(GL_TRIANGLES, entity.mesh->mesh.count, GL_UNSIGNED_INT, 0);
			}
		}
	}

	//Directional Shadow
	glUseProgram(m_DirectionalShadowShaderID);
	const std::vector<LightInfo>& dirInfos = m_LightBuffers[DIRECTIONAL_SHADOWED_LIGHT].GetInfos();
	for (size_t i = 0; i < dirInfos.size(); ++i) {
		DirLightShadow<5>::IntArray update;

		std::vector<glm::mat4> _transforms = getLightSpaceMatrices(glm::normalize(dirInfos[i].direction), camera);
		DirLightShadow<5>::Mat4Array transforms;
		for (int i = 0; i < transforms.size(); ++i) {
			transforms[i] = _transforms[i];
		}
		if (!dirShadows[i].Update(dirInfos[i].refreshFrequency, update, dirInfos[i], transforms)) continue;

		dirShadows[i].bind(dirInfos[i]);

		glUniform1iv(6, 5, update.data());
		glUniformMatrix4fv(1, 5, GL_FALSE, (GLfloat*)dirShadows[i].transforms.data());
		for (const auto& entity : entities) {
			if (entity.castShadow) {
				glm::mat4 matrix = entity.GetLocalModelMatrix();
				glUniformMatrix4fv(0, 1, GL_FALSE, glm::value_ptr(matrix));
				glBindVertexArray(entity.mesh->mesh.vaoID);
				glDrawElements(GL_TRIANGLES, entity.mesh->mesh.count, GL_UNSIGNED_INT, 0);
			}
		}
	}

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport(windowValues[0], windowValues[1], windowValues[2], windowValues[3]);
}

void Lights::renderPointLights(GLuint diffuseBuffer, GLuint normalBuffer, GLuint depthBuffer, const Camera& camera, LightType type) const {
	assert(type == POINT_LIGHT || type == POINT_SHADOWED_LIGHT);
	m_LightBuffers[type].Bind(0);

	glDepthFunc(GL_GREATER);
	glDepthMask(GL_FALSE);

	glUseProgram(m_LightShaderIDs[POINT_LIGHT]);

	glBindTextureUnit(1, diffuseBuffer);
	glBindTextureUnit(2, normalBuffer);
	glBindTextureUnit(3, depthBuffer);

	glUniformMatrix4fv(0, 1, GL_FALSE, glm::value_ptr(camera.GetViewMatrix()));
	glUniformMatrix4fv(1, 1, GL_FALSE, glm::value_ptr(camera.GetProj()));
	glUniformMatrix4fv(2, 1, GL_FALSE, glm::value_ptr(glm::inverse(camera.GetProj())));

	glPatchParameteri(GL_PATCH_VERTICES, 1);
	glDrawArraysInstanced(GL_PATCHES, 0, m_LightBuffers[type].GetSize(), 1);

	glDepthMask(GL_TRUE);
	glDepthFunc(GL_LESS);
}

void Lights::renderDirectionalLights(GLuint diffuseBuffer, GLuint normalBuffer, const Camera& camera, LightType type) const {
	assert(type == DIRECTIONAL_LIGHT || type == DIRECTIONAL_SHADOWED_LIGHT);
	m_LightBuffers[type].Bind(0);
	glDisable(GL_DEPTH_TEST);
	GLuint program = m_LightShaderIDs[DIRECTIONAL_LIGHT];
	glUseProgram(program);
	glBindTextureUnit(1, diffuseBuffer);
	glBindTextureUnit(2, normalBuffer);
	glm::mat4 matrix = glm::inverse(camera.GetViewMatrix());
	glUniformMatrix4fv(0, 1, GL_TRUE, glm::value_ptr(matrix));

	glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, m_LightBuffers[type].GetSize());
	glEnable(GL_DEPTH_TEST);
}

void Lights::renderPointLightsShadowed(GLuint diffuseBuffer, GLuint normalBuffer, GLuint depthBuffer, const Camera& camera) const {
	glDepthFunc(GL_GREATER);
	glDepthMask(GL_FALSE);

	GLuint program = m_LightShaderIDs[POINT_SHADOWED_LIGHT];
	glUseProgram(program);
	glBindTextureUnit(1, diffuseBuffer);
	glBindTextureUnit(2, normalBuffer);
	glBindTextureUnit(3, depthBuffer);

	glm::mat4 matrix = glm::inverse(camera.GetProj());
	glUniformMatrix4fv(4, 1, GL_FALSE, glm::value_ptr(matrix));
	matrix = camera.GetViewMatrix();
	glUniformMatrix4fv(0, 1, GL_FALSE, glm::value_ptr(matrix));
	matrix = camera.GetProj();
	glUniformMatrix4fv(2, 1, GL_FALSE, glm::value_ptr(matrix));
	matrix = glm::inverse(camera.GetViewMatrix());
	glUniformMatrix4fv(5, 1, GL_FALSE, glm::value_ptr(matrix));

	const std::vector<LightInfo>& infos = m_LightBuffers[POINT_SHADOWED_LIGHT].GetInfos();
	for (size_t i = 0; i < infos.size(); ++i) {
		glBindTextureUnit(0, pointShadows[i].GetTexture());
		glUniform3fv(6, 1, glm::value_ptr(infos[i].color));
		glUniform3fv(3, 1, glm::value_ptr(infos[i].position));
		glUniform1f(1, pointShadows[i].GetRadius());
		glDrawArrays(GL_PATCHES, 0, 1);
	}
	
	glDepthMask(GL_TRUE);
	glDepthFunc(GL_LESS);
}

void Lights::renderDirectionalLightsShadowed(GLuint diffuseBuffer, GLuint normalBuffer, GLuint depthBuffer, const Camera& camera) const {
	glDisable(GL_DEPTH_TEST);

	GLuint program = m_LightShaderIDs[DIRECTIONAL_SHADOWED_LIGHT];
	glUseProgram(program);
	glBindTextureUnit(0, diffuseBuffer);
	glBindTextureUnit(1, normalBuffer);
	glBindTextureUnit(2, depthBuffer);

	glm::mat4 matrix = camera.GetViewMatrix();
	glUniformMatrix4fv(2, 1, GL_FALSE, glm::value_ptr(matrix));
	matrix = glm::inverse(camera.GetProj());
	glUniformMatrix4fv(0, 1, GL_FALSE, glm::value_ptr(matrix));
	matrix = glm::inverse(camera.GetViewMatrix());
	glUniformMatrix4fv(1, 1, GL_FALSE, glm::value_ptr(matrix));

	const std::vector<LightInfo>& infos = m_LightBuffers[DIRECTIONAL_SHADOWED_LIGHT].GetInfos();
	for (size_t i = 0; i < infos.size(); ++i) {
		std::vector<glm::mat4> transforms = getLightSpaceMatrices(glm::normalize(infos[i].direction), camera);
		glUniformMatrix4fv(5, 5, GL_FALSE, (float*)transforms.data());
		glBindTextureUnit(3, dirShadows[i].depthTexture);
		glUniform3fv(3, 1, glm::value_ptr(infos[i].color));
		glm::vec3 direction = glm::normalize(infos[i].direction);
		glUniform3fv(4, 1, glm::value_ptr(direction));
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	}

	glEnable(GL_DEPTH_TEST);
}

void Lights::CreateFrameBuffer(GLint width, GLint height, GLuint depthTexture) {
	if (m_FrameBufferID) {
		glDeleteTextures(1, &m_TextureID);
		glDeleteFramebuffers(1, &m_FrameBufferID);
	}

	glCreateFramebuffers(1, &m_FrameBufferID);

	glCreateTextures(GL_TEXTURE_2D, 1, &m_TextureID);

	glTextureStorage2D(m_TextureID, 1, GL_RGBA32F, width, height);
	glTextureParameterf(m_TextureID, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTextureParameterf(m_TextureID, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTextureParameterf(m_TextureID, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTextureParameterf(m_TextureID, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	glNamedFramebufferTexture(m_FrameBufferID, GL_COLOR_ATTACHMENT0, m_TextureID, 0);
	CheckGlError("Error creating color attachment");

	glNamedFramebufferTexture(m_FrameBufferID, GL_DEPTH_STENCIL_ATTACHMENT, depthTexture, 0);
	CheckGlError("Error creating depth attachment");

	CheckFramebufferError(m_FrameBufferID);
}

GLuint Lights::GetLightTexture() const {
	return m_TextureID;
}

void Lights::AddLight(LightType type, const LightInfo& info) {
	m_LightBuffers[type].AddLight(info);
	switch (type)
	{
	case POINT_SHADOWED_LIGHT:
		pointShadows.push_back(info);
		break;
	case DIRECTIONAL_SHADOWED_LIGHT:
		dirShadows.push_back(info);
		break;
	default:
		break;
	}
}

void Lights::UpdateLight(LightType type, size_t index) {
	m_LightBuffers[type].UpdateLight(index);
	switch (type)
	{
	case POINT_SHADOWED_LIGHT:
		pointShadows[index].Clean();
		pointShadows[index] = m_LightBuffers[POINT_SHADOWED_LIGHT].GetInfos()[index];
		break;
	case DIRECTIONAL_SHADOWED_LIGHT:
		dirShadows[index].Clean();
		dirShadows[index] = m_LightBuffers[DIRECTIONAL_SHADOWED_LIGHT].GetInfos()[index];
		break;
	default:
		break;
	}
}

void Lights::DeleteLight(LightType type, size_t index) {
	m_LightBuffers[type].DeleteLight(index);
	switch (type)
	{
	case POINT_SHADOWED_LIGHT:
		pointShadows[index].Clean();
		pointShadows.erase(pointShadows.begin() + index);
		break;
	case DIRECTIONAL_SHADOWED_LIGHT:
		dirShadows[index].Clean();
		dirShadows.erase(dirShadows.begin() + index);
		break;
	default:
		break;
	}
}

void Lights::ChangeShadowed(LightType type, size_t index) {
	LightInfo current = m_LightBuffers[type].GetInfos()[index];
	DeleteLight(type, index);
	switch (type)
	{
	case POINT_LIGHT:
		AddLight(POINT_SHADOWED_LIGHT, current);
		break;
	case DIRECTIONAL_LIGHT:
		AddLight(DIRECTIONAL_SHADOWED_LIGHT, current);
		break;
	case POINT_SHADOWED_LIGHT:
		AddLight(POINT_LIGHT, current);
		break;
	case DIRECTIONAL_SHADOWED_LIGHT:
		AddLight(DIRECTIONAL_LIGHT, current);
	}
}

std::vector<LightInfo>& Lights::GetInfo(LightType type) {
	return m_LightBuffers[type].GetInfos();
}

std::vector<glm::vec4> Lights::getFrustumCornersWorldSpace(const glm::mat4& projView) const {
	const auto inv = glm::inverse(projView);

	std::vector<glm::vec4> frustumCorners;
	for (unsigned int x = 0; x < 2; ++x)
	{
		for (unsigned int y = 0; y < 2; ++y)
		{
			for (unsigned int z = 0; z < 2; ++z)
			{
				const glm::vec4 pt =
					inv * glm::vec4(
						2.0f * x - 1.0f,
						2.0f * y - 1.0f,
						2.0f * z - 1.0f,
						1.0f);
				frustumCorners.push_back(pt / pt.w);
			}
		}
	}

	return frustumCorners;
}

glm::mat4 Lights::getLightSpaceMatrix(const float nearPlane, const float farPlane, const glm::vec3& lightDir, const Camera& camera) const {
	const std::vector<glm::vec4> corners = getFrustumCornersWorldSpace(camera.GetFrustumRange(nearPlane, farPlane));

	glm::vec3 center = glm::vec3(0, 0, 0);
	for (const auto& v : corners)
	{
		center += glm::vec3(v);
	}
	center /= corners.size();

	const auto lightView = glm::lookAt(center + lightDir, center, glm::vec3(0.0f, 1.0f, 0.0f));

	float minX = std::numeric_limits<float>::max();
	float maxX = std::numeric_limits<float>::lowest();
	float minY = std::numeric_limits<float>::max();
	float maxY = std::numeric_limits<float>::lowest();
	float minZ = std::numeric_limits<float>::max();
	float maxZ = std::numeric_limits<float>::lowest();
	for (const auto& v : corners)
	{
		const glm::vec4 trf = lightView * v;
		minX = std::min(minX, trf.x);
		maxX = std::max(maxX, trf.x);
		minY = std::min(minY, trf.y);
		maxY = std::max(maxY, trf.y);
		minZ = std::min(minZ, trf.z);
		maxZ = std::max(maxZ, trf.z);
	}

	// Tune this parameter according to the scene
	constexpr float zMult = 10.0f;
	if (minZ < 0) {
		minZ *= zMult;
	}
	else {
		minZ /= zMult;
	}
	if (maxZ < 0) {
		maxZ /= zMult;
	}
	else {
		maxZ *= zMult;
	}

	const glm::mat4 lightProjection = glm::ortho(minX, maxX, minY, maxY, minZ, maxZ);
	return lightProjection * lightView;
}

std::vector<glm::mat4> Lights::getLightSpaceMatrices(const glm::vec3& lightDir, const Camera& camera) const {
	std::vector<glm::mat4> ret;
	for (size_t i = 0; i < shadowCascadeLevels.size() + 1; ++i)
	{
		if (i == 0)
		{
			ret.push_back(getLightSpaceMatrix(camera.GetZNear(), shadowCascadeLevels[i], lightDir, camera));
		}
		else if (i < shadowCascadeLevels.size())
		{
			ret.push_back(getLightSpaceMatrix(shadowCascadeLevels[i - 1], shadowCascadeLevels[i], lightDir, camera));
		}
		else
		{
			ret.push_back(getLightSpaceMatrix(shadowCascadeLevels[i - 1], camera.GetZFar(), lightDir, camera));
		}
	}
	return ret;
}