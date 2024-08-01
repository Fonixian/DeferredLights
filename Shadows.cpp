#include "Shadows.h"
#include <SDL2/SDL.h>
#include <string>

PointLightShadow::PointLightShadow(const LightInfo& info) : m_range(sqrt((info.color.r + info.color.g + info.color.b) * 25)), m_refreshTime(0.0f) {
	glCreateTextures(GL_TEXTURE_CUBE_MAP, 1, &m_TextureID);
	glTextureStorage2D(m_TextureID, 1, GL_DEPTH_COMPONENT24, info.shadowMapResolutionWH[0], info.shadowMapResolutionWH[1]);
	
	glTextureParameteri(m_TextureID, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTextureParameteri(m_TextureID, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTextureParameteri(m_TextureID, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTextureParameteri(m_TextureID, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTextureParameteri(m_TextureID, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

	
	glCreateFramebuffers(1, &m_FrameBufferID);
	glNamedFramebufferTexture(m_FrameBufferID, GL_DEPTH_ATTACHMENT, m_TextureID, 0);
	CheckGlError("Error creating depth attachment");

	glNamedFramebufferDrawBuffer(m_FrameBufferID, GL_NONE);
	glNamedFramebufferReadBuffer(m_FrameBufferID, GL_NONE);

	CheckFramebufferError(m_FrameBufferID);
}

void PointLightShadow::ClearShadow(int lower, int upper, const LightInfo& info) {
	if (upper == 0) return;
	static float clearValue = 1.0f;

	glClearTexSubImage(m_TextureID, 0, 0, 0, lower, info.shadowMapResolutionWH[0], info.shadowMapResolutionWH[0],
		upper - lower, GL_DEPTH_COMPONENT, GL_FLOAT, &clearValue);
}

bool PointLightShadow::Update(float Frequency, std::array<int, 6>& updateValues, const LightInfo& info) {
	bool update = (int)(m_refreshTime + 6.0f / Frequency) > (int)m_refreshTime;
	int lower = (int)m_refreshTime;
	m_refreshTime += 6.0f / Frequency;
	int upper = (int)m_refreshTime % 6;

	m_refreshTime = std::fmod(m_refreshTime, 6.0f);
	if (!update) return update;

	updateValues.fill(0);
	if (lower < upper) {
		for (int i = lower; i < upper; ++i)updateValues[i] = 1;
		ClearShadow(lower, upper, info);
	}
	else if (lower == upper) {
		updateValues.fill(1);
		ClearShadow(0, 6, info);
	}
	else {
		for (int i = lower; i < 6; ++i)updateValues[i] = 1;
		for (int i = 0; i < upper; ++i)updateValues[i] = 1;
		ClearShadow(lower, 6, info);
		ClearShadow(0, upper, info);
	}

	return update;
}

void PointLightShadow::Clean() {
	glDeleteTextures(1, &m_TextureID);
	glDeleteFramebuffers(1, &m_FrameBufferID);
}

void PointLightShadow::Bind(const LightInfo& info) const {
	glViewport(0, 0, info.shadowMapResolutionWH[0], info.shadowMapResolutionWH[0]);
	glBindFramebuffer(GL_FRAMEBUFFER, m_FrameBufferID);
}

GLuint PointLightShadow::GetTexture() const {
	return m_TextureID;
}

float PointLightShadow::GetRadius() const {
	return m_range;
}