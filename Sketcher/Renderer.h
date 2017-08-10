#pragma once

#include <mutex>
#include <functional>
#include <vector>

#include <glm\glm.hpp>
#include <glm\gtc\matrix_transform.hpp>
#include <glm\gtc\matrix_inverse.hpp>
#include <glm\gtc\type_ptr.hpp>

#include <GL\gl3w.h>
#include <GLFW\glfw3.h>
#include <imgui.h>

#include "Arcball.h"
#include "MyMesh.h"
#include "CommonDefs.h"
#include "MyShader.h"

class Renderer {
public:
	static Renderer& Instance() {
		static Renderer instance;
		return instance;
	}
	void Terminate();
private:
	Renderer();
	~Renderer() {};

public:
	void cursorpos_callback(GLFWwindow* win, double x, double y);
	void mouseButton_callback(GLFWwindow* win, int button, int action, int modifiers);
	void scroll_callback(GLFWwindow* win, double x_offset, double y_offset);
	void Resize(int width, int height);

	void DrawInterface(bool * p_open = nullptr);
	void DrawCameraInterface(bool* p_open = nullptr);

	// Causes renderer to upload the geometry in the next frame
	void InvalidateGeometry() { updated_geometry_ = false; }
	void InvalidateChainTypes();

	template<typename Matrix>
	void UploadShaderAttrib(enum Render mode, std::string name, Matrix& M) {
		shader[mode].shader.Bind();
		shader[mode].shader.UploadAttrib(name, M);
	}

	void Render();

	void ResetCamera();

	int Width() const { return width; }
	int Height() const { return height; }

	BasicShader* GetShader(enum Render mode) { return shader[mode]; }
	FaceShader* GetFaceShader(enum Render mode) { return face_shader[mode]; }
	EdgeShader* GetEdgeShader(enum Render mode) { return edge_shader[mode]; }
	CustomShader* GetCustomShader(enum Render mode) { return custom_shader[mode]; }
	HalfedgeShader* GetHalfedgeShader(enum Render mode) { return halfedge_shader[mode]; }

	void SetSolidColor();
	void SetWireframeColor();
	void SetFeatureColor();
	void SetBoundaryColor();

public:
	// picker stuff
	bool moved_since_mouse_press = false;
	bool active[Render::N_RENDER_MODES];
	VertexHandle picked_vertex;
	EdgeHandle picked_edge;
	FaceHandle picked_face;

	ImColor solidcolor = ImColor(0.4f, 0.4f, 0.4f);
	ImColor wirecolor = ImColor(0.8f, 0.8f, 0.8f, 0.3f);
	ImColor featurecolor = ImColor(1.0f, 0.0f, 0.0f);
	ImColor boundarycolor = ImColor(0.0f, 0.0f, 1.0f);
	ImColor segmentationcolor = ImColor(0.0f, 0.0f, 0.0f);

	bool line_depth_test = true;
	float wire_thickness_factor = 0.125f;
	float line_thickness_factor = 0.15f;

	bool normal_lines = true;
	bool flipped_lines = false;

	std::vector<int> selected_edge_id;
	std::vector<ImColor> selected_edge_color;

	// turntable stuff
	float rotation_amount = 0.0f;
	glm::vec3 rotation_center = glm::vec3(0.0f, 0.0f, 0.0f);
	glm::vec3 rotation_axis = glm::vec3(0.0f, 1.0f, 0.0f);
private:
	void UploadMeshData();
	void UpdateLineThickness();

	// returns screen coords in range [-1, 1], [-1, 1]
	glm::vec2 GetScreenCoords(double x, double y) const {
		return glm::vec2(2 * x / width - 1, 2 * y / height - 1);
	}

	// camera parameters
	float fov_;
	float nearplane_;
	float farplane_;

	int width;
	int height;

	glm::vec3 eye;
	glm::vec3 center;
	glm::vec3 up;

	bool lock_camera = false;

	float scale_;
	bool rotate_active_;
	bool translate_xy_active_;
	bool translate_z_active_;

	glm::vec3 translate_;
	float translate_speed_ = 0.01f;
	glm::vec2 translate_start_;
	
	Arcball arcball;

	// phong shader parameters
	float phong_ambient_intensity = 0.3f;
	float phong_diffuse_intensity = 1.0f;
	float phong_specular_intensity = 1.0f;
	float phong_specular_shininess = 15.0f;
	float l0Intensity = 1.0f;
	float l1Intensity = 0.5f;
	float l2Intensity = 0.7f;
	glm::vec3 l0Position = glm::vec3(6, 3, 3);
	glm::vec3 l1Position = glm::vec3(6, 0, -3);
	glm::vec3 l2Position = glm::vec3(-6, 0, 2);;
	glm::vec3 DiffuseMaterial = glm::vec3(0.6f, 0.5f, 0.4f);
	glm::vec3 AmbientMaterial = glm::vec3(0.2f, 0.2f, 0.2f);
	glm::vec3 SpecularMaterial = glm::vec3(0.3f, 0.2f, 0.1f);

	// OpenGL matrices
	glm::mat4 view;
	glm::mat4 model;
	glm::mat4 projection;
	glm::mat4 modelViewProj;
	glm::mat4 modelView;
	glm::mat3 normalmatrix;

	BasicShader* shader[Render::N_RENDER_MODES];
	FaceShader* face_shader[Render::N_RENDER_MODES];
	EdgeShader* edge_shader[Render::N_RENDER_MODES];
	CustomShader* custom_shader[Render::N_RENDER_MODES];
	HalfedgeShader* halfedge_shader[Render::N_RENDER_MODES];

	std::vector<enum Render> face_shader_list;
	std::vector<enum Render> edge_shader_list;
	std::vector<enum Render> halfedge_shader_list;
	std::vector<enum Render> normal_shader_list;

	bool updated_geometry_;
};
