#pragma once
#include "Camera.h"
#include "Lights.h"
#include "Logs.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

template<int size>
class DirLightShadow {
public:
	using Mat4Array = std::array<glm::mat4, size>;
	using IntArray = std::array<int, size>;

	GLuint frameBuffer;
	GLuint depthTexture;
	Mat4Array transforms;
	float refreshTime;

	DirLightShadow(const LightInfo& info) : refreshTime(0.0f) {
		createTextures(info);
	}

	bool Update(float Frequency, IntArray& updateValues, const LightInfo& info, const Mat4Array& transforms) {
		bool update = (int)(refreshTime + 6.0f / Frequency) > (int)refreshTime;
		int lower = (int)refreshTime;
		refreshTime += 6.0f / Frequency;
		int upper = (int)refreshTime % 6;

		refreshTime = std::fmod(refreshTime, 6.0f);
		if (!update) return update;

		updateValues.fill(0);
		if (lower < upper) {
			for (int i = lower; i < upper; ++i)updateValues[i] = 1;
			clearShadow(lower, upper, info);
		} else if (lower == upper) {
			updateValues.fill(1);
			clearShadow(0, size, info);
		} else {
			for (int i = lower; i < size; ++i)updateValues[i] = 1;
			for (int i = 0; i < upper; ++i)updateValues[i] = 1;
			clearShadow(lower, size, info);
			clearShadow(0, upper, info);
		}

		updateTransforms(updateValues, transforms);
		return update;
	}

	void Clean() {
		glDeleteTextures(1, &depthTexture);
		glDeleteFramebuffers(1, &frameBuffer);
	}

	void bind(const LightInfo& info) const {
		glViewport(0, 0, info.shadowMapResolutionWH[0], info.shadowMapResolutionWH[1]);
		glBindFramebuffer(GL_FRAMEBUFFER, frameBuffer);
	}

private:

	void clearShadow(int lower, int upper, const LightInfo& info) { //[lower, upper[
		if (upper == 0) return;
		static float clearValue = 1.0f;
		glClearTexSubImage(depthTexture, 0, 0, 0, lower, info.shadowMapResolutionWH[0], info.shadowMapResolutionWH[1],
			upper - lower, GL_DEPTH_COMPONENT, GL_FLOAT, &clearValue);
	}

	void createTextures(const LightInfo& info) {
		glCreateFramebuffers(1, &frameBuffer);
		
		glCreateTextures(GL_TEXTURE_2D_ARRAY, 1, &depthTexture);
		glTextureStorage3D(depthTexture, 1, GL_DEPTH_COMPONENT24, info.shadowMapResolutionWH[0], info.shadowMapResolutionWH[1], size); // GL_DEPTH_COMPONENT32
		
		glTextureParameteri(depthTexture, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTextureParameteri(depthTexture, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTextureParameteri(depthTexture, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
		glTextureParameteri(depthTexture, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

		constexpr float bordercolor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
		glTextureParameterfv(depthTexture, GL_TEXTURE_BORDER_COLOR, bordercolor);

		glNamedFramebufferTexture(frameBuffer, GL_DEPTH_ATTACHMENT, depthTexture, 0);
		CheckGlError("Error creating depth attachment");
		
		glNamedFramebufferDrawBuffer(frameBuffer, GL_NONE);
		glNamedFramebufferReadBuffer(frameBuffer, GL_NONE);

		CheckFramebufferError(frameBuffer);
	}

	void updateTransforms(const IntArray& updates, const Mat4Array& transforms) {
		for (size_t i = 0; i < size; ++i) if (updates[i] == 1)this->transforms[i] = transforms[i];
	}
};

class PointLightShadow {
private:
	GLuint m_FrameBufferID;
	GLuint m_TextureID;
	float m_range;
	float m_refreshTime;

	void ClearShadow(int lower, int upper, const LightInfo& info);
public:
	PointLightShadow(const LightInfo& info);
	bool Update(float Frequency, std::array<int, 6>& updateValues, const LightInfo& info);
	void Clean();
	void Bind(const LightInfo& info) const;
	GLuint GetTexture() const;
	float GetRadius() const;
};