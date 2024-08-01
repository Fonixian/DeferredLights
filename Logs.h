#pragma once
#include <GL/glew.h>
#include <SDL2/SDL.h>
#include <string>

template<bool exitOnFail = true>
void CheckGlError(const char* msg) {
	if (glGetError() != GL_NO_ERROR) {
		SDL_Log(msg);
		if constexpr (exitOnFail) exit(1);
	}
}

template<bool exitOnFail = true>
void CheckFramebufferError(GLuint frameBuffer) {
	GLenum status = glCheckNamedFramebufferStatus(frameBuffer, GL_FRAMEBUFFER);
	if (status != GL_FRAMEBUFFER_COMPLETE) {
		std::string msg = "Incomplete framebuffer (";
		switch (status) {
		case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:			msg += "GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT";		   break;
		case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:	msg += "GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT"; break;
		case GL_FRAMEBUFFER_UNSUPPORTED:					msg += "GL_FRAMEBUFFER_UNSUPPORTED";				   break;
		}
		msg += ")";
		SDL_Log(msg.c_str());
		if constexpr (exitOnFail) exit(1);
	}
}