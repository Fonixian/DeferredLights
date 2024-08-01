#pragma once
#include <GL/glew.h>
#include "Camera.h"

class SSAO {
	GLuint m_Framebuffer = 0;
	GLuint m_Texture = 0;
	GLuint m_NoiseTexture = 0;

	GLuint m_ProgramID = 0;;

public:
	SSAO();
	~SSAO();

	void RenderSSAO(GLuint, GLuint, const Camera&);
	void CreateFrameBuffer(GLint, GLint);
	GLuint GetSSAO();

};