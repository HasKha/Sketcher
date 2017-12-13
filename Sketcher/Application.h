#pragma once

#include <string>
#include <thread>
#include <functional>
#include <queue>
#include <sstream>
#include <iomanip>

#include <Eigen/Core>
#include <GL/gl3w.h>
#include <GLFW/glfw3.h>

#include "CommonDefs.h"
#include "Renderer.h"

class Application {
public:
	Application();
	~Application() {}

	void DrawInterface(GLFWwindow* window);
	void PostRender();

	void LoadMeshDialog();
	void LoadMesh(const std::string filename);
	void ResetAll();

	void ToggleInterface() { show_interface = !show_interface; }

	bool save_screenshot;

private:

	bool show_interface;
	bool show_controls;
	bool show_visualization;
	bool show_console;
	bool show_test_window;
	bool show_camera;

	bool move_mesh_to_origin = true;
	bool normalize_mesh = true;
};
