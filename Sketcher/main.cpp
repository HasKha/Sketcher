/*
This file serves as an entry point and handles command line argument(s),
initialization of the window, handling of main loop, and IO callbacks
*/

#include <stdio.h>

#include <GL/gl3w.h>
#include <GLFW/glfw3.h>

#include <imgui.h>
#include <examples/opengl3_example/imgui_impl_glfw_gl3.h>

#include "Worker.h"
#include "Renderer.h"
#include "Application.h"
#include "Log.h"

Application* app = nullptr;

static void error_callback(int error, const char* desc);
static void size_callback(GLFWwindow* win, int w, int h);
static void mouseButton_callback(GLFWwindow* win, int button, int action, int mod);
static void cursorPos_callback(GLFWwindow* win, double x, double y);
static void scroll_callback(GLFWwindow* win, double x_offset, double y_offset);
static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode);
static void char_callback(GLFWwindow* window, unsigned int c);

#ifdef _WIN32
int CALLBACK WinMain(
	_In_ HINSTANCE hInstance,
	_In_ HINSTANCE hPrevInstance,
	_In_ LPSTR     lpCmdLine,
	_In_ int       nCmdShow
) {
#else
int main(int argc, char **argv) {
#endif
	glfwSetErrorCallback(error_callback);
	if (!glfwInit())
		return 1;

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	glfwWindowHint(GLFW_SAMPLES, 4);

	int width = 1920;
	int height = 1020;
	GLFWwindow* window = glfwCreateWindow(width, height, "Sketcher", NULL, NULL);
	//glfwSetWindowPos(window, -1920, 25);
	
	glfwMakeContextCurrent(window);
	
	gl3wInit(); // CRASH HERE

	app = new Application();
	Renderer::Instance().Resize(width, height);

	ImGui_ImplGlfwGL3_Init(window, false);

#ifdef _WIN32
	// Set imgui's ini file in %APPDATA% in windows. 
	// TODO: make it work on unix too
	char* appdata = getenv("APPDATA");
	char buf[256];
	sprintf_s(buf, "%s\\sketcher.ini", appdata);
	ImGui::GetIO().IniFilename = buf;
#endif

	glfwSetMouseButtonCallback(window, mouseButton_callback);
	glfwSetCursorPosCallback(window, cursorPos_callback);
	glfwSetScrollCallback(window, scroll_callback);
	glfwSetKeyCallback(window, key_callback);
	glfwSetCharCallback(window, char_callback);
	glfwSetFramebufferSizeCallback(window, size_callback);

	Worker::Do([]() {}); // start up the worker
	Renderer::Instance(); // initialize the renderer

	while (!glfwWindowShouldClose(window)) {
		glfwPollEvents();
		ImGui_ImplGlfwGL3_NewFrame();

		app->DrawInterface(window);
		
		Renderer::Instance().Render();

		ImGui::Render();
		glfwSwapBuffers(window);

		app->PostRender();
	}

	// Cleanup
	Renderer::Instance().Terminate();
	Worker::Stop();
	
	ImGui_ImplGlfwGL3_Shutdown();
	glfwTerminate();

	return 0;
}

static void error_callback(int error, const char* description) {
	print("Error %d: %s\n", error, description);
}

static void size_callback(GLFWwindow* win, int w, int h) {
	Renderer::Instance().Resize(w, h);
}

static void mouseButton_callback(GLFWwindow* win, int button, int action, int mod) {
	ImGui_ImplGlfwGL3_MouseButtonCallback(win, button, action, mod);
	if (ImGui::GetIO().WantCaptureMouse) return;
	Renderer::Instance().mouseButton_callback(win, button, action, mod);
}

static void cursorPos_callback(GLFWwindow* win, double x, double y) {
	if (ImGui::GetIO().WantCaptureMouse) return;
	Renderer::Instance().cursorpos_callback(win, x, y);
}

static void scroll_callback(GLFWwindow* win, double x_offset, double y_offset) {
	ImGui_ImplGlfwGL3_ScrollCallback(win, x_offset, y_offset);
	if (ImGui::GetIO().WantCaptureMouse) return;
	Renderer::Instance().scroll_callback(win, x_offset, y_offset);
}

static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode) {
	ImGui_ImplGlfwGL3_KeyCallback(window, key, scancode, action, mode);
	if (ImGui::GetIO().WantTextInput) return;

	if (action == GLFW_RELEASE) {
		switch (key) {
		case GLFW_KEY_S:
			app->save_screenshot = true;
			break;
		case GLFW_KEY_O:
			app->LoadMeshDialog();
			break;
		case GLFW_KEY_I:
			app->ToggleInterface();
			break;
		case GLFW_KEY_R:
			app->ResetAll();
		default:
			break;
		}
	}
}

static void char_callback(GLFWwindow* window, unsigned int c) {
	ImGui_ImplGlfwGL3_CharCallback(window, c);
	if (ImGui::GetIO().WantTextInput) return;
}
