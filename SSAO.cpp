#include "SSAO.h"
#include "ProgramBuilder.h"
#include "Logs.h"
#include <random>
#include <glm/gtc/type_ptr.hpp>

inline float lerp(float a, float b, float f) {
    return a + f * (b - a);
}

SSAO::SSAO() : m_ProgramID(glCreateProgram()) {
    
    ProgramBuilder{ m_ProgramID }
        .ShaderStage(GL_VERTEX_SHADER, "Shaders/SSAO.vert")
        .ShaderStage(GL_FRAGMENT_SHADER, "Shaders/SSAO.frag")
        .Link();

    std::uniform_real_distribution<float> random_m1_p1(-1.0, 1.0);
    std::uniform_real_distribution<float> random_p0_p1(0.0, 1.0);
    std::default_random_engine generator;
    std::vector<glm::vec3> ssaoKernel;
    for (unsigned int i = 0; i < 64; ++i)
    {
        glm::vec3 sample(
            random_m1_p1(generator),
            random_m1_p1(generator),
            random_p0_p1(generator)
        );
        sample = glm::normalize(sample);
        sample *= random_p0_p1(generator);

        float scale = (float)i / 64.0f;
        scale = lerp(0.1f, 1.0f, scale * scale);
        sample *= scale;
        ssaoKernel.push_back(sample);
    }

    glUseProgram(m_ProgramID);
    glUniform3fv(3, 64, (const GLfloat*)ssaoKernel.data());
    glUseProgram(0);

    std::vector<glm::vec3> ssaoNoise;
    for (unsigned int i = 0; i < 16; i++)
    {
        glm::vec3 noise;
        while (true) {
            noise = glm::vec3(
                random_m1_p1(generator),
                random_m1_p1(generator),
                0.0f);
            if (glm::length(noise) != 0)break;
        }
        ssaoNoise.push_back(noise);
    }

    glCreateTextures(GL_TEXTURE_2D, 1, &m_NoiseTexture);
    glTextureStorage2D(m_NoiseTexture, 1, GL_RGBA16F, 4, 4);
    glTextureSubImage2D(m_NoiseTexture, 0, 0, 0, 4, 4, GL_RGB, GL_FLOAT, ssaoNoise.data());
    glTextureParameteri(m_NoiseTexture, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTextureParameteri(m_NoiseTexture, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTextureParameteri(m_NoiseTexture, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTextureParameteri(m_NoiseTexture, GL_TEXTURE_WRAP_T, GL_REPEAT);
}

SSAO::~SSAO() {
    if (m_Framebuffer) {
        glDeleteTextures(1, &m_Framebuffer);
        glDeleteFramebuffers(1, &m_Texture);
    }
    glDeleteTextures(1, &m_NoiseTexture);
}

void SSAO::RenderSSAO(const GLuint normalBuffer, const GLuint depthBuffer, const Camera& camera) {
    glBindFramebuffer(GL_FRAMEBUFFER, m_Framebuffer);
    glUseProgram(m_ProgramID);
    glBindTextureUnit(0, normalBuffer);
    glBindTextureUnit(1, depthBuffer);
    glBindTextureUnit(2, m_NoiseTexture);
    glm::mat4 matrix = camera.GetProj();
    glUniformMatrix4fv(0, 1, GL_FALSE, glm::value_ptr(matrix));
    matrix = glm::inverse(matrix);
    glUniformMatrix4fv(1, 1, GL_FALSE, glm::value_ptr(matrix));
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

void SSAO::CreateFrameBuffer(GLint width, GLint height) {
    if (m_Framebuffer) {
        glDeleteTextures(1, &m_Texture);
        glDeleteFramebuffers(1, &m_Framebuffer);
    }

    glCreateFramebuffers(1, &m_Framebuffer);
    glCreateTextures(GL_TEXTURE_2D, 1, &m_Texture);

    glTextureStorage2D(m_Texture, 1, GL_R8, width, height);

    glTextureParameteri(m_Texture, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTextureParameteri(m_Texture, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTextureParameteri(m_Texture, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTextureParameteri(m_Texture, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glNamedFramebufferTexture(m_Framebuffer, GL_COLOR_ATTACHMENT0, m_Texture, 0);
    CheckGlError("Error creating color attachment 0");
    CheckFramebufferError(m_Framebuffer);

    glUseProgram(m_ProgramID);
    glUniform2f(2, width / 4.0f, height / 4.0f);
    glUseProgram(0);
}

GLuint SSAO::GetSSAO() {
    return m_Texture;
}