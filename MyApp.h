#pragma once
#include <array>
#include <memory>

// GLM
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/transform.hpp>

// GLEW
#include <GL/glew.h>

// SDL
#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>

// Utils
#include "Camera.h"
#include "CameraManipulator.h"
#include "GLUtils.hpp"

#include "Entity.h"
#include "Lights.h"
#include "SSAO.h"

struct SUpdateInfo
{
	float ElapsedTimeInSec = 0.0f;	// Elapsed time since start of the program
	float DeltaTimeInSec = 0.0f;	// Elapsed time since last update
};

class CMyApp
{
public:
	CMyApp();
	~CMyApp();

	bool Init();
	void Clean();

	void Update(const SUpdateInfo&);
	void Render();
	void RenderGUI();

	void KeyboardDown(const SDL_KeyboardEvent&);
	void KeyboardUp(const SDL_KeyboardEvent&);
	void MouseMove(const SDL_MouseMotionEvent&);
	void MouseDown(const SDL_MouseButtonEvent&);
	void MouseUp(const SDL_MouseButtonEvent&);
	void MouseWheel(const SDL_MouseWheelEvent&);
	void Resize(int, int);

	void OtherEvent(const SDL_Event&);
protected:
	void SetupDebugCallback();
	void RenderEntityGUI();
	void RenderLightGUI(LightType);

	Lights m_lights;
	SSAO m_SSAO;

	//
	// Variables
	//

	// Entities
	void InitEntities();

	std::vector<Entity> m_entities;

	// Camera
	Camera m_camera;
	CameraManipulator m_cameraManipulator;

	//
	// OpenGL
	//

	void DrawAxes();
	void DrawSkybox();
	// Uniform location query
	static GLint ul(const char* uniformName) noexcept;

	// Shader variables
	GLuint m_programAxesID = 0;		// Axes program
	GLuint m_programSkyboxID = 0;	// Skybox program
	GLuint m_programPostProcessID = 0;
	GLuint m_programNonReflectiveID = 0;
	GLuint m_programReflectiveID = 0;


	// Shader initialization and termination
	void InitShaders();
	void CleanShaders();
	void InitAxesShader();
	void CleanAxesShader();
	void InitSkyboxShaders();
	void CleanSkyboxShaders();

	// Geometry variables
	OGLObject m_skybox = {};
	Mesh m_suzanne = {};
	Mesh m_sphere = {};
	Mesh m_cube = {};
	Mesh m_tree = {};
	Mesh m_surface = {};

	// Geometry initialization and termination
	void InitGeometry();
	void CleanGeometry();
	void InitSkyboxGeometry();
	void CleanSkyboxGeometry();

	// Texture variables
	GLuint m_skyboxTextureID = 0;
	GLuint m_metalTextureID = 0;
	GLuint m_grassTextureID = 0;
	GLuint m_treeTextureID = 0;

	// Texture initialization and termination
	void InitTextures();
	void CleanTextures();
	void InitSkyboxTextures();
	void CleanSkyboxTextures();

	// Framebuffer variables
	GLuint m_sceneFrameBuffer = 0;
	GLuint m_diffuseTextureID = 0;
	GLuint m_normalTextureID = 0; // view
	GLuint m_depthTextureID = 0; // perspective

	// Framebuffer initialization and termination
	void CreateFramebuffer(GLint, GLint);

	void DrawScene(const glm::mat4&, const glm::mat4&, bool) const;
};