#include "MyApp.h"
#include "SDL_GLDebugMessageCallback.h"
#include "ParametricSurfaceMesh.hpp"
#include "ProgramBuilder.h"
#include "ObjParser.h"
#include "Logs.h"

#include <imgui.h>
#include <iostream>
#include <array>

class BezierSurface {
	const std::array<glm::vec3, 16> controllPoints = {
		glm::vec3(-200, 20, -200), glm::vec3(-200, 0, -100), glm::vec3(-200,  -10, 0), glm::vec3(-200, 20, 100),
		glm::vec3(-100, 30, -200), glm::vec3(-100, 40, -100), glm::vec3(-100,  0, 0), glm::vec3(-100, 0, 100),
		glm::vec3(0, 9, -200), glm::vec3(0, 40, -100), glm::vec3(0,  30, 0), glm::vec3(0, 0, 100),
		glm::vec3(100, 13, -200), glm::vec3(100, 2, -100), glm::vec3(100, -2, 0), glm::vec3(100, 0, 100)
	};

public:
	glm::vec3 GetPos(float u, float v) const {
		float x = u * 300.f - 200.f;
		float z = v * 300.f - 200.f;

		float Bu[4] = { (1 - u) * (1 - u) * (1 - u),3 * u * (1 - u) * (1 - u),3 * u * u * (1 - u),u * u * u };
		float Bv[4] = { (1 - v) * (1 - v) * (1 - v),3 * v * (1 - v) * (1 - v),3 * v * v * (1 - v),v * v * v };

		float y = 0;
		for (int i = 0; i < 4; ++i)
			for (int j = 0; j < 4; ++j)
				y += controllPoints[i + j * 4].y * Bu[i] * Bv[j];

		return { x, y, z };
	}

	glm::vec3 GetNorm(float u, float v) const {
		float Bu[4] = { (1 - u) * (1 - u) * (1 - u),3 * u * (1 - u) * (1 - u),3 * u * u * (1 - u),u * u * u };
		float Bv[4] = { (1 - v) * (1 - v) * (1 - v),3 * v * (1 - v) * (1 - v),3 * v * v * (1 - v),v * v * v };

		float dBu[3] = { (1 - u) * (1 - u), 2 * u * (1 - u),u * u };
		float dBv[3] = { (1 - v) * (1 - v), 2 * v * (1 - v),v * v };

		glm::vec3 dub[3 * 4];
		glm::vec3 dvb[4 * 3];

		for (int i = 0; i < 3; ++i)
			for (int j = 0; j < 4; ++j)
				dub[j * 3 + i] = 3.0f * (controllPoints[i + 1 + j * 4] - controllPoints[j * 4 + i]);

		for (int i = 0; i < 4; ++i)
			for (int j = 0; j < 3; ++j)
				dvb[j * 4 + i] = 3.0f * (controllPoints[i + (j + 1) * 4] - controllPoints[j * 4 + i]);


		glm::vec3 du(0, 0, 0);
		glm::vec3 dv(0, 0, 0);

		for (int i = 0; i < 3; ++i)
			for (int j = 0; j < 4; ++j)
				du += dub[j * 3 + i] * dBu[i] * Bv[j];

		for (int i = 0; i < 4; ++i)
			for (int j = 0; j < 3; ++j)
				dv += dvb[j * 4 + i] * Bu[i] * dBv[j];

		return glm::normalize(cross(du, dv));
	}

	glm::vec2 GetTex(float u, float v) const {
		return { u, v };
	}

	float getHeight(float x, float z) const {
		float height = 0.0f;
		float u = (x + 200.f) / 300.f;
		float v = (z + 200.f) / 300.f;

		float Bu[4] = { (1 - u) * (1 - u) * (1 - u),3 * u * (1 - u) * (1 - u),3 * u * u * (1 - u),u * u * u };
		float Bv[4] = { (1 - v) * (1 - v) * (1 - v),3 * v * (1 - v) * (1 - v),3 * v * v * (1 - v),v * v * v };

		for (int i = 0; i < 4; ++i)
			for (int j = 0; j < 4; ++j)
				height += controllPoints[i + j * 4].y * Bu[i] * Bv[j];

		return height;
	}
};

CMyApp::CMyApp()
{
}

CMyApp::~CMyApp()
{
}

void CMyApp::SetupDebugCallback()
{
	// Enable and set the debug callback function if we are in debug context
	GLint context_flags;
	glGetIntegerv(GL_CONTEXT_FLAGS, &context_flags);
	if (context_flags & GL_CONTEXT_FLAG_DEBUG_BIT) {
		glEnable(GL_DEBUG_OUTPUT);
		glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
		glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_NOTIFICATION, 0, nullptr, GL_FALSE);
		glDebugMessageCallback(SDL_GLDebugMessageCallback, nullptr);
	}
}

void CMyApp::InitEntities() {
	BezierSurface surface;
	m_entities.emplace_back(&m_surface, m_grassTextureID, glm::vec3(0.f), glm::vec3(0.f), glm::vec3(1.f)).castShadow = false;

	for (int i = -1; i <= 1; ++i)
		for (int j = -1; j <= 1; ++j)
		{
			if ((i + j) % 2 == 0) {
				m_entities.emplace_back(&m_suzanne, m_metalTextureID, glm::vec3(50 + 4 * i, 4 * (j + 1) + surface.getHeight(50, -80) + 5.f, -80), glm::vec3(0.f), glm::vec3(1.f));
			} else {
				m_entities.emplace_back(&m_sphere, m_grassTextureID, glm::vec3(50 + 4 * i, 4 * (j + 1) + surface.getHeight(50, -80) + 5.f, -80), glm::vec3(0.f), glm::vec3(1.f));
			}
		}

	const float x[5] = { -180.f, -80.f, -150.f, -30.f, -164.f };
	const float z[5] = { -180.f, -30.f, -90.f, -100.f, -54.f };
	for (int i = 0; i < 5; ++i)
		m_entities.emplace_back(&m_tree, m_treeTextureID, glm::vec3(x[i], surface.getHeight(x[i], z[i]), z[i]) - 0.2f, glm::vec3(0, x[i], 0), glm::vec3(1));

	m_entities.emplace_back(&m_cube, m_metalTextureID, glm::vec3(50, surface.getHeight(50, -80) + 1.f, -80), glm::vec3(0.f), glm::vec3(20, 2, 20));


	m_entities.emplace_back(&m_cube, m_metalTextureID, glm::vec3(0, surface.getHeight(0, 10.0f) + 6.f, 0), glm::vec3(0), glm::vec3(3));
	m_entities.emplace_back(&m_cube, m_metalTextureID, glm::vec3(0, surface.getHeight(0, 12.5f) + 6.f, 12.5f), glm::vec3(0), glm::vec3(1));

	m_entities[m_entities.size() - 1].SetGenerateReflection(true);
	m_entities[m_entities.size() - 2].SetGenerateReflection(true);
}

void CMyApp::InitShaders()
{
	m_programNonReflectiveID = glCreateProgram();
	ProgramBuilder{ m_programNonReflectiveID }
		.ShaderStage(GL_VERTEX_SHADER, "Shaders/myVert.vert")
		.ShaderStage(GL_FRAGMENT_SHADER, "Shaders/myFrag.frag")
		.Link();

	m_programReflectiveID = glCreateProgram();
	ProgramBuilder{ m_programReflectiveID }
		.ShaderStage(GL_VERTEX_SHADER, "Shaders/reflective.vert")
		.ShaderStage(GL_FRAGMENT_SHADER, "Shaders/reflective.frag")
		.Link();

	m_programPostProcessID = glCreateProgram();
	ProgramBuilder{ m_programPostProcessID }
		.ShaderStage(GL_VERTEX_SHADER, "Shaders/postprocess.vert")
		.ShaderStage(GL_FRAGMENT_SHADER, "Shaders/postprocess.frag")
		.Link();

	InitAxesShader();
	InitSkyboxShaders();
}

void CMyApp::CleanShaders()
{
	glDeleteProgram(m_programNonReflectiveID);
	glDeleteProgram(m_programReflectiveID);
	glDeleteProgram(m_programPostProcessID);
	CleanAxesShader();
	CleanSkyboxShaders();
}

void CMyApp::InitAxesShader()
{
	m_programAxesID = glCreateProgram();
	ProgramBuilder{ m_programAxesID }
		.ShaderStage(GL_VERTEX_SHADER, "Shaders/Vert_axes.vert")
		.ShaderStage(GL_FRAGMENT_SHADER, "Shaders/Frag_Col.frag")
		.Link();
}

void CMyApp::CleanAxesShader()
{
	glDeleteProgram(m_programAxesID);
}

void CMyApp::InitSkyboxShaders()
{
	m_programSkyboxID = glCreateProgram();
	ProgramBuilder{ m_programSkyboxID }
		.ShaderStage(GL_VERTEX_SHADER, "Shaders/Vert_skybox.vert")
		.ShaderStage(GL_FRAGMENT_SHADER, "Shaders/Frag_skybox.frag")
		.Link();
}

void CMyApp::CleanSkyboxShaders()
{
	glDeleteProgram(m_programSkyboxID);
}

void getBoundingSphere(const MeshObject<Vertex>& meshData, Mesh& mesh) {
	glm::vec3 center = glm::vec3(0, 0, 0);
	for (const Vertex& vertex : meshData.vertexArray) {
		center += vertex.position;
	}
	mesh.center = center / static_cast<float>(meshData.vertexArray.size());
}

void CMyApp::InitGeometry()
{
	const std::initializer_list<VertexAttributeDescriptor> vertexAttribList =
	{
		{ 0, offsetof(Vertex, position), 3, GL_FLOAT },
		{ 1, offsetof(Vertex, normal),   3, GL_FLOAT },
		{ 2, offsetof(Vertex, texcoord), 2, GL_FLOAT },
	};

	MeshObject<Vertex> suzanneMeshCPU = ObjParser::parse("Assets/Suzanne.obj");
	m_suzanne.mesh = CreateGLObjectFromMesh(suzanneMeshCPU, vertexAttribList);
	getBoundingSphere(suzanneMeshCPU, m_suzanne);

	MeshObject<Vertex> sphereMeshCPU = ObjParser::parse("Assets/sphere.obj");
	m_sphere.mesh = CreateGLObjectFromMesh(sphereMeshCPU, vertexAttribList);
	getBoundingSphere(sphereMeshCPU, m_sphere);

	MeshObject<Vertex> cubeMeshCPU = ObjParser::parse("Assets/cube.obj");
	m_cube.mesh = CreateGLObjectFromMesh(cubeMeshCPU, vertexAttribList);
	getBoundingSphere(cubeMeshCPU, m_cube);

	MeshObject<Vertex> treeMeshCPU = ObjParser::parse("Assets/tree2.obj");
	m_tree.mesh = CreateGLObjectFromMesh(treeMeshCPU, vertexAttribList);
	getBoundingSphere(treeMeshCPU, m_tree);

	MeshObject<Vertex> surfaceMeshCPU = GetParamSurfMesh(BezierSurface{}, 100, 100);
	m_surface.mesh = CreateGLObjectFromMesh(surfaceMeshCPU, vertexAttribList);

	InitSkyboxGeometry();
}

void CMyApp::CleanGeometry()
{
	CleanOGLObject(m_suzanne.mesh);
	CleanOGLObject(m_sphere.mesh);
	CleanOGLObject(m_cube.mesh);
	CleanOGLObject(m_tree.mesh);
	CleanOGLObject(m_surface.mesh);
	CleanSkyboxGeometry();
}

void CMyApp::InitSkyboxGeometry()
{
	MeshObject<glm::vec3> skyboxCPU
	{
		std::vector<glm::vec3>
		{
			// Back side
			glm::vec3(-1, -1, -1),
			glm::vec3(1, -1, -1),
			glm::vec3(1,  1, -1),
			glm::vec3(-1,  1, -1),
			// Front side
			glm::vec3(-1, -1,  1),
			glm::vec3(1, -1,  1),
			glm::vec3(1,  1,  1),
			glm::vec3(-1,  1,  1),
		}, // 8 corner of the cube

		std::vector<GLuint>
		{
			// Back side
			0, 1, 2,
			2, 3, 0,
			// Front side
			4, 6, 5,
			6, 4, 7,
			// Left side
			0, 3, 4,
			4, 3, 7,
			// Right side
			1, 5, 2,
			5, 6, 2,
			// Bottom side
			1, 0, 4,
			1, 4, 5,
			// Top side
			3, 2, 6,
			3, 6, 7,
		}
	};

	m_skybox = CreateGLObjectFromMesh(skyboxCPU, { { 0, offsetof(glm::vec3, x), 3, GL_FLOAT } });
}

void CMyApp::CleanSkyboxGeometry()
{
	CleanOGLObject(m_skybox);
}

void CMyApp::InitTextures()
{
	std::vector<const char*> paths = {"Assets/metal.png", "Assets/grass.png", "Assets/tree.bmp"};
	std::vector<GLuint*> locations = { &m_metalTextureID, &m_grassTextureID, &m_treeTextureID };

	for (size_t i = 0; i < paths.size(); ++i) {
		glGenTextures(1, locations[i]);
		TextureFromFile(*locations[i], paths[i]);
		SetupTextureSampling(GL_TEXTURE_2D, *locations[i]);
	}

	InitSkyboxTextures();
}

void CMyApp::CleanTextures()
{
	std::vector<GLuint*> locations = { &m_metalTextureID, &m_grassTextureID, &m_treeTextureID };
	for (GLuint* currentTexture : locations) glDeleteTextures(1, currentTexture);
	CleanSkyboxTextures();
}

void CMyApp::InitSkyboxTextures()
{
	glGenTextures(1, &m_skyboxTextureID);

	std::array<std::filesystem::path, 6> paths{ "Assets/xpos.png", "Assets/xneg.png", "Assets/ypos.png", "Assets/yneg.png", "Assets/zpos.png", "Assets/zneg.png" };
	for (Uint32 i = 0; i < paths.size(); ++i)
	{
		TextureFromFile(m_skyboxTextureID, paths[i], GL_TEXTURE_CUBE_MAP, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i);
	}

	SetupTextureSampling(GL_TEXTURE_CUBE_MAP, m_skyboxTextureID, false);

	glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
}

void CMyApp::CleanSkyboxTextures()
{
	glDeleteTextures(1, &m_skyboxTextureID);
}

bool CMyApp::Init()
{
	SetupDebugCallback();

	glClearColor(0.125f, 0.25f, 0.5f, 0.0f);

	InitShaders();
	InitGeometry();
	InitTextures();
	InitEntities();

	//
	// Other
	//

	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);
	glEnable(GL_DEPTH_TEST);
	// Camera
	m_camera.SetView(
		glm::vec3(10, 40, 10),
		glm::vec3(0, 15, 0),
		glm::vec3(0, 1, 0)
	);
	m_cameraManipulator.SetCamera(&m_camera);
	
	return true;
}

void CMyApp::Clean()
{
	CleanShaders();
	CleanGeometry();
	CleanTextures();
}

void CMyApp::Update(const SUpdateInfo& updateInfo)
{
	m_cameraManipulator.Update(updateInfo.DeltaTimeInSec);
}

void CMyApp::DrawAxes()
{
	// We always want to see it, regardless of whether there is an object in front of it
	glDisable(GL_DEPTH_TEST);
	glUseProgram(m_programAxesID);

	glm::mat4 axisWorld = glm::translate(m_camera.GetAt());
	glUniformMatrix4fv(ul("viewProj"), 1, GL_FALSE, glm::value_ptr(m_camera.GetViewProj()));
	glUniformMatrix4fv(ul("world"), 1, GL_FALSE, glm::value_ptr(axisWorld));

	glDrawArrays(GL_LINES, 0, 6);
	glEnable(GL_DEPTH_TEST);
	glUseProgram(0);
}

void CMyApp::DrawSkybox()
{
	glBindTextureUnit(0, m_skyboxTextureID);
	glUseProgram(m_programSkyboxID);

	glUniform1i( ul("skyboxTexture"), 0 );
	glUniformMatrix4fv(	ul("viewProj"),	1, GL_FALSE, glm::value_ptr(m_camera.GetViewProj()) );
	glUniformMatrix4fv(	ul("world"),	1, GL_FALSE, glm::value_ptr(glm::translate(m_camera.GetEye())) );

	GLint prevDepthFnc;
	glGetIntegerv(GL_DEPTH_FUNC, &prevDepthFnc);

	glDepthFunc(GL_LEQUAL);

	glBindVertexArray(m_skybox.vaoID);
	glDrawElements(GL_TRIANGLES, m_skybox.count, GL_UNSIGNED_INT, nullptr);

	glDepthFunc(prevDepthFnc);

	glUseProgram(0);
	glBindVertexArray(0);
}

void CMyApp::Render()
{
	GLint windowValues[4];
	glGetIntegerv(GL_VIEWPORT, windowValues);
	for (Entity& entity : m_entities) entity.Update(m_entities);
	glViewport(windowValues[0], windowValues[1], windowValues[2], windowValues[3]);

	// Lights
	m_lights.UpdateShadowMaps(m_entities, m_camera);
	glUseProgram(0);

	glBindFramebuffer(GL_FRAMEBUFFER, m_sceneFrameBuffer);
	glStencilMask(0xFF);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
	DrawScene(m_camera.GetProj(), m_camera.GetViewMatrix(), true);
	DrawScene(m_camera.GetProj(), m_camera.GetViewMatrix(), false);

	m_lights.RenderLights(m_diffuseTextureID, m_normalTextureID, m_depthTextureID, m_camera);

	// SSAO
	m_SSAO.RenderSSAO(m_normalTextureID, m_depthTextureID, m_camera);
	
	// Draw
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	DrawSkybox();

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glUseProgram(m_programPostProcessID);
	glBindTextureUnit(0, m_lights.GetLightTexture());
	glBindTextureUnit(1, m_SSAO.GetSSAO());
	glBindTextureUnit(2, m_diffuseTextureID);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	glDisable(GL_BLEND);

	DrawAxes();
}

void CMyApp::RenderGUI()
{
	// ImGui::ShowDemoWindow();

	ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Once);
	ImGui::SetNextWindowSize(ImVec2(640, 480), ImGuiCond_Once);
	if (ImGui::Begin("Menu"))
	{
		glm::vec3 position = m_camera.GetEye();
		ImGui::Text("X: %f Y: %f Z: %f", position.x, position.y, position.z);
		ImGui::Separator();
		if (ImGui::BeginTabBar("Tabs", 0))
		{
			if (ImGui::BeginTabItem("Textures"))
			{
				ImGui::Image((ImTextureID)m_diffuseTextureID, ImVec2(256, 256), ImVec2(0, 1), ImVec2(1, 0));
				ImGui::Image((ImTextureID)m_normalTextureID, ImVec2(256, 256), ImVec2(0, 1), ImVec2(1, 0));
				ImGui::Image((ImTextureID)m_SSAO.GetSSAO(), ImVec2(256, 256), ImVec2(0, 1), ImVec2(1, 0));
				ImGui::EndTabItem();
			}
			if (ImGui::BeginTabItem("Objects"))
			{
				RenderEntityGUI();
				ImGui::EndTabItem();
			}
			if (ImGui::BeginTabItem("Lights"))
			{
				if (ImGui::CollapsingHeader("Point Light")) {
					if (ImGui::Button("New point light")) {
						m_lights.AddLight(POINT_LIGHT, { {10,10,10}, { m_camera.GetEye(), 1 }, false, { 1024,1024 }
					});
					}
					RenderLightGUI(POINT_LIGHT);
					RenderLightGUI(POINT_SHADOWED_LIGHT);
				}

				if (ImGui::CollapsingHeader("Directional Light")) {
					if (ImGui::Button("New directional light")) {
						m_lights.AddLight(DIRECTIONAL_LIGHT);
					}
					RenderLightGUI(DIRECTIONAL_LIGHT);
					RenderLightGUI(DIRECTIONAL_SHADOWED_LIGHT);
				}
				ImGui::EndTabItem();
			}
			ImGui::EndTabBar();
		}
	}
	ImGui::End();
}

void CMyApp::RenderEntityGUI() {
	for (Entity& entity : m_entities) {
		ImGui::PushID(&entity);
		if (ImGui::InputFloat3("Position: X Y Z", glm::value_ptr(entity.position))) {
			entity.Moved();
		}
		ImGui::InputFloat3("Rotation: X Y Z", glm::value_ptr(entity.rotation));
		ImGui::InputFloat3("Scale: X Y Z", glm::value_ptr(entity.scale));
		ImGui::Checkbox("Cast Shadow", &entity.castShadow);
		ImGui::Checkbox("Receive Shadow", &entity.receiveShadow);
		ImGui::Checkbox("Refleced", &entity.reflected);
		if (!entity.GetGenerateReflection()) {
			if (ImGui::Button("Reflection")) {
				entity.SetGenerateReflection(true);
			}
		}
		else {
			if (ImGui::InputInt("Resolution", &entity.environmentMap->resolution)) {
				entity.environmentMap->createFrameBuffer(entity.environmentMap->resolution);
			}
			ImGui::InputFloat("Frequency", &entity.environmentMap->frequency);
			if (ImGui::Button("Reflection")) {
				entity.SetGenerateReflection(false);
			}
		}

		ImGui::PopID();
		ImGui::Separator();
	}
}

void CMyApp::RenderLightGUI(LightType type) {
	bool shadowChange = false;
	size_t index;

	std::vector<LightInfo>& currentLights = m_lights.GetInfo(type);
	for (size_t i = 0; i < currentLights.size(); ++i) {
		ImGui::PushID(&currentLights[i]);

		if (ImGui::InputFloat3("R G B", (float*)&currentLights[i].color)) {
			m_lights.UpdateLight(type, i);
		}
		if (ImGui::InputFloat3("X Y Z", (float*)&currentLights[i].position)) {
			m_lights.UpdateLight(type, i);
		}
		if (ImGui::Checkbox("Shadow", &currentLights[i].castShadow)) {
			shadowChange = true;
			index = i;
		}
		switch (type)
		{
		case POINT_LIGHT:
		case POINT_SHADOWED_LIGHT:
			if (ImGui::InputInt("Shadow resolution", currentLights[i].shadowMapResolutionWH.data())) {
				m_lights.UpdateLight(type, i);
			}
			break;
		case DIRECTIONAL_LIGHT:
		case DIRECTIONAL_SHADOWED_LIGHT:
			if (ImGui::InputInt2("Shadow resolution", currentLights[i].shadowMapResolutionWH.data())) {
				m_lights.UpdateLight(type, i);
			}
			break;
		}
		ImGui::DragFloat("Refresh ", &currentLights[i].refreshFrequency, .25f, 1.0f, 10.f);
		if (ImGui::Button("Delete")) {
			m_lights.DeleteLight(type, i);
		}

		ImGui::PopID();
		ImGui::Separator();
	}

	if (shadowChange)m_lights.ChangeShadowed(type, index);
}

GLint CMyApp::ul(const char* uniformName) noexcept
{
	GLuint programID = 0;

	// Get the current program ID
	// https://registry.khronos.org/OpenGL-Refpages/gl4/html/glGet.xhtml
	glGetIntegerv(GL_CURRENT_PROGRAM, reinterpret_cast<GLint*>(&programID));
	// Knowing the program and the name of the uniform, let's get the location!
	// https://registry.khronos.org/OpenGL-Refpages/gl4/html/glGetUniformLocation.xhtml
	return glGetUniformLocation(programID, uniformName);
}

// https://wiki.libsdl.org/SDL2/SDL_KeyboardEvent
// https://wiki.libsdl.org/SDL2/SDL_Keysym
// https://wiki.libsdl.org/SDL2/SDL_Keycode
// https://wiki.libsdl.org/SDL2/SDL_Keymod

void CMyApp::KeyboardDown(const SDL_KeyboardEvent& key)
{
	if (key.repeat == 0) // Triggers only once when held
	{
		if (key.keysym.sym == SDLK_F5 && key.keysym.mod & KMOD_CTRL) // CTRL + F5
		{
			CleanShaders();
			InitShaders();
		}
		if (key.keysym.sym == SDLK_F1) // F1
		{
			GLint polygonModeFrontAndBack[2] = {};
			// https://registry.khronos.org/OpenGL-Refpages/gl4/html/glGet.xhtml
			glGetIntegerv(GL_POLYGON_MODE, polygonModeFrontAndBack); // Query the current polygon mode. It gives the front and back modes separately.
			GLenum polygonMode = (polygonModeFrontAndBack[0] != GL_FILL ? GL_FILL : GL_LINE); // Switch between FILL and LINE
			// https://registry.khronos.org/OpenGL-Refpages/gl4/html/glPolygonMode.xhtml
			glPolygonMode(GL_FRONT_AND_BACK, polygonMode); // Set the new polygon mode
		}
	}
	m_cameraManipulator.KeyboardDown(key);
}

void CMyApp::KeyboardUp(const SDL_KeyboardEvent& key)
{
	m_cameraManipulator.KeyboardUp(key);
}

// https://wiki.libsdl.org/SDL2/SDL_MouseMotionEvent

void CMyApp::MouseMove(const SDL_MouseMotionEvent& mouse)
{
	m_cameraManipulator.MouseMove(mouse);
}

// https://wiki.libsdl.org/SDL2/SDL_MouseButtonEvent

void CMyApp::MouseDown(const SDL_MouseButtonEvent& mouse)
{
}

void CMyApp::MouseUp(const SDL_MouseButtonEvent& mouse)
{
}

// https://wiki.libsdl.org/SDL2/SDL_MouseWheelEvent

void CMyApp::MouseWheel(const SDL_MouseWheelEvent& wheel)
{
	m_cameraManipulator.MouseWheel(wheel);
}

// New window size
void CMyApp::Resize(int _w, int _h)
{
	glViewport(0, 0, _w, _h);
	m_camera.SetAspect(static_cast<float>(_w) / _h);
	CreateFramebuffer(_w, _h);
}

// Other SDL events
// https://wiki.libsdl.org/SDL2/SDL_Event

void CMyApp::OtherEvent(const SDL_Event& ev)
{
}

void CMyApp::DrawScene(const glm::mat4& proj, const glm::mat4& view, bool receiveShadow) const {
	glm::mat4 viewProj = proj * view;
	glm::mat4 world;

	if (!receiveShadow) {
		glEnable(GL_STENCIL_TEST);
		glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
		glStencilFunc(GL_ALWAYS, 1, 0xFF);
		glStencilMask(0xFF);
	}

	glUseProgram(m_programNonReflectiveID);
	for (const Entity& entity : m_entities) {
		if (entity.receiveShadow == receiveShadow && !entity.GetGenerateReflection()) {
			entity.SetTexture(m_programNonReflectiveID);
			world = entity.GetLocalModelMatrix();

			/*glm::mat4 matrix = viewProj * world;
			glUniformMatrix4fv(1, 1, GL_FALSE, glm::value_ptr(matrix));
			matrix = glm::transpose(glm::inverse(world));
			glUniformMatrix4fv(0, 1, GL_FALSE, glm::value_ptr(matrix));
			matrix = glm::transpose(glm::inverse(view * world));
			glUniformMatrix4fv(2, 1, GL_FALSE, glm::value_ptr(matrix));*/
			glUniformMatrix4fv(0, 1, GL_FALSE, glm::value_ptr(glm::transpose(glm::inverse(world))));
			glUniformMatrix4fv(1, 1, GL_FALSE, glm::value_ptr(viewProj * world));
			glUniformMatrix4fv(2, 1, GL_FALSE, glm::value_ptr(glm::transpose(glm::inverse(view * world))));

			glBindVertexArray(entity.mesh->mesh.vaoID);
			glDrawElements(GL_TRIANGLES, entity.mesh->mesh.count, GL_UNSIGNED_INT, 0);
		}
	}
	
	glUseProgram(m_programReflectiveID);
	for (const Entity& entity : m_entities) {
		if (entity.receiveShadow == receiveShadow && entity.GetGenerateReflection()) {
			glBindTextureUnit(1, entity.environmentMap->getTexture());
			entity.SetTexture(m_programReflectiveID);
			world = entity.GetLocalModelMatrix();

			glm::mat4 matrix = m_camera.GetViewMatrix() * world;
			glUniformMatrix4fv(3, 1, GL_FALSE, glm::value_ptr(matrix));
			matrix = glm::inverse(m_camera.GetViewMatrix());
			glUniformMatrix4fv(4, 1, GL_FALSE, glm::value_ptr(matrix));
			matrix = viewProj * world;
			glUniformMatrix4fv(1, 1, GL_FALSE, glm::value_ptr(matrix));
			matrix = glm::transpose(glm::inverse(world));
			glUniformMatrix4fv(0, 1, GL_FALSE, glm::value_ptr(matrix));
			matrix = glm::transpose(glm::inverse(m_camera.GetViewMatrix() * world));
			glUniformMatrix4fv(2, 1, GL_FALSE, glm::value_ptr(matrix));

			glBindVertexArray(entity.mesh->mesh.vaoID);
			glDrawElements(GL_TRIANGLES, entity.mesh->mesh.count, GL_UNSIGNED_INT, 0);
		}
	}

	if (!receiveShadow) {
		glDisable(GL_STENCIL_TEST);
	}

	Entity::Reset();
}

void CMyApp::CreateFramebuffer(GLint width, GLint height) {
	if (m_sceneFrameBuffer) {
		glDeleteTextures(1, &m_diffuseTextureID);
		glDeleteTextures(1, &m_normalTextureID);
		glDeleteTextures(1, &m_depthTextureID);
		glDeleteFramebuffers(1, &m_sceneFrameBuffer);
	}

	glCreateFramebuffers(1, &m_sceneFrameBuffer);
	// Diffuse
	glCreateTextures(GL_TEXTURE_2D, 1, &m_diffuseTextureID);
	glTextureStorage2D(m_diffuseTextureID, 1, GL_RGBA8, width, height);

	glTextureParameteri(m_diffuseTextureID, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTextureParameteri(m_diffuseTextureID, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTextureParameteri(m_diffuseTextureID, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTextureParameteri(m_diffuseTextureID, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	glNamedFramebufferTexture(m_sceneFrameBuffer, GL_COLOR_ATTACHMENT0, m_diffuseTextureID, 0);
	CheckGlError("Error creating color attachment 0");
	// Normal
	glCreateTextures(GL_TEXTURE_2D, 1, &m_normalTextureID);
	glTextureStorage2D(m_normalTextureID, 1, GL_RGB16F, width, height);

	glTextureParameteri(m_normalTextureID, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTextureParameteri(m_normalTextureID, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTextureParameteri(m_normalTextureID, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTextureParameteri(m_normalTextureID, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	glNamedFramebufferTexture(m_sceneFrameBuffer, GL_COLOR_ATTACHMENT1, m_normalTextureID, 0);
	CheckGlError("Error creating color attachment 1");
	// Depth
	glCreateTextures(GL_TEXTURE_2D, 1, &m_depthTextureID);
	glTextureStorage2D(m_depthTextureID, 1, GL_DEPTH24_STENCIL8, width, height);

	glTextureParameteri(m_depthTextureID, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTextureParameteri(m_depthTextureID, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTextureParameteri(m_depthTextureID, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTextureParameteri(m_depthTextureID, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	glNamedFramebufferTexture(m_sceneFrameBuffer, GL_DEPTH_STENCIL_ATTACHMENT, m_depthTextureID, 0);
	CheckGlError("Error creating color attachment 1");

	GLenum drawBuffers[2] = { GL_COLOR_ATTACHMENT0,
							  GL_COLOR_ATTACHMENT1 };
	glNamedFramebufferDrawBuffers(m_sceneFrameBuffer, 2, drawBuffers);

	CheckFramebufferError(m_sceneFrameBuffer);

	m_lights.CreateFrameBuffer(width, height, m_depthTextureID);
	m_SSAO.CreateFrameBuffer(width, height);
}