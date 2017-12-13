#include "Application.h"
#include <Eigen/Core>

#include <iostream>
#include <fstream>
#include <time.h>

#include <imgui.h>
#include "Converters.h"
#include "Worker.h"
#include "Log.h"
#include "FileDialog.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

Application::Application() :
	save_screenshot(false),
	show_interface(true),
	show_controls(true),
	show_visualization(true),
	show_console(true),
	show_test_window(false), 
	show_camera(false) {

	// uncomment to specify mesh to load at startup
	//Worker::Do([&]() {
	//	LoadMesh("../../Meshes/mouse.obj");
	//});
}

void Application::DrawInterface(GLFWwindow* window) {
	if (!show_interface) return;

	if (ImGui::BeginMainMenuBar()) {
		if (ImGui::BeginMenu("Menu")) {
			if (ImGui::MenuItem("Save Image", "S")) {
				save_screenshot = true;
			}
			if (ImGui::MenuItem("Open Mesh", "O")) {
				LoadMeshDialog();
			}
			if (ImGui::MenuItem("Toggle Interface", "I")) {
				ToggleInterface();
			}
			if (ImGui::MenuItem("Quit")) { glfwSetWindowShouldClose(window, GL_TRUE); }
			ImGui::EndMenu();
		}
		ImGui::Checkbox("Controls", &show_controls);
		ImGui::SameLine();
		ImGui::Checkbox("Visualization", &show_visualization);
		ImGui::SameLine();
		ImGui::Checkbox("Console", &show_console);
		ImGui::SameLine();
		ImGui::Checkbox("Camera", &show_camera);
		ImGui::SameLine();
		//if (ImGui::BeginMenu("Window")) {
		//	ImGui::MenuItem("Controls", "", &show_controls);
		//	ImGui::MenuItem("Visualization", "", &show_visualization);
		//	ImGui::MenuItem("Console", "", &show_console);
		//	ImGui::MenuItem("Camera", "", &show_camera);
		//	ImGui::MenuItem("Test Window", "", &show_test_window);
		//	ImGui::EndMenu();
		//}
		ImGui::SameLine(ImGui::GetWindowWidth() - 80);
		ImGui::Text("(%.0f fps)", ImGui::GetIO().Framerate);
		ImGui::EndMainMenuBar();
	}

	if (show_test_window) {
		ImGui::ShowTestWindow(&show_test_window);
	}

	if (show_visualization) {
		Renderer::Instance().DrawInterface(&show_visualization);
	}

	if (show_controls) {
		const ImVec2 button_size(180, 0);
		ImGui::SetNextWindowSize(ImVec2(300, 680), ImGuiSetCond_FirstUseEver);
		ImGui::Begin("Controls", &show_controls);
		ImGui::PushItemWidth(180);
		if (ImGui::Button("Open Mesh (O)", button_size)) {
			LoadMeshDialog();
		}
		if (ImGui::IsItemHovered()) ImGui::SetTooltip("Supported file types: Obj or OFF");
		ImGui::SameLine();
		if (ImGui::TreeNode("Options##openmesh")) {
			ImGui::Checkbox("Move mesh to origin on load", &move_mesh_to_origin);
			ImGui::Checkbox("Normalize mesh on load", &normalize_mesh);
			if (ImGui::SmallButton("Move mesh to origin now")) {
				Worker::Do([]() { 
					mesh().move_to_origin();
					Renderer::Instance().InvalidateGeometry();
				});
			}
			if (ImGui::SmallButton("Normalize mesh now")) {
				Worker::Do([]() { 
					mesh().normalize();
					Renderer::Instance().InvalidateGeometry();
				});
			}
			ImGui::TreePop();
		}
		if (ImGui::Button("Save Mesh", button_size)) {
			Worker::Do([&]() {
				std::string name = file_dialog({ {"obj", "obj"} }, true);
				if (!name.empty()) {
					print("Saving mesh to %s\n", name.c_str());
					OpenMesh::IO::write_mesh(cmesh(), name + ".obj");
					print("Saving complete\n");
				}
			});
		}
		if (ImGui::Button("Reset All (R)", button_size)) {
			ResetAll();
		}
		if (ImGui::IsItemHovered())	ImGui::SetTooltip("Reset everything");

		ImGui::Separator();
		if (ImGui::Button("Reset Geometry", button_size)) {
			Worker::Do([&]() {
				print("Unsmoothing...\n");
				Renderer::Instance().active[Wireframe] = true;
				for (const VertexHandle v : cmesh().vertices()) {
					mesh().set_point(v, cbackup().point(v));
				}
				Renderer::Instance().InvalidateGeometry();
				print("Unsmoothing done\n");
			});
		}
		if (ImGui::IsItemHovered()) ImGui::SetTooltip("Reset vertex positions");

		if (ImGui::SliderFloat("Feature max angle", &(mesh().feature_max_angle), 0, 181, "%.0f deg")) {
			Renderer::Instance().GetShader(FeaturesEdges)->Invalidate();
		}
		if (ImGui::IsItemHovered()) ImGui::SetTooltip("Feature edge dihedral angle threshold");

		ImGui::End();
	}

	if (show_console) {
		Log::Instance().Draw("Log", &show_console);
	}

	if (show_camera) {
		Renderer::Instance().DrawCameraInterface(&show_camera);
	}
}

void Application::ResetAll() {
	Worker::Do([&]() {
		mesh().render_ready = false;
		RestoreBackup();
		mesh().initialize();
		mesh().render_ready = true;
		Renderer::Instance().InvalidateGeometry();
	});
}

void Application::PostRender() {
	if (save_screenshot) {
		save_screenshot = false;
		const unsigned int bytesPerPixel = 4; // RGBx
		const int width = Renderer::Instance().Width();
		const int height = Renderer::Instance().Height();
		unsigned char* pixels = new unsigned char[bytesPerPixel * width * height];
		glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
		for (int y = 0; y < height / 2; ++y) {
			const int y2 = height - y - 1;
			for (int x = 0; x < width; ++x) {
				const int offset1 = bytesPerPixel * (x + y * width);
				const int offset2 = bytesPerPixel * (x + y2 * width);
				std::swap(pixels[offset1 + 0], pixels[offset2 + 0]);
				std::swap(pixels[offset1 + 1], pixels[offset2 + 1]);
				std::swap(pixels[offset1 + 2], pixels[offset2 + 2]);
			}
		}
		for (int y = 0; y < height; ++y) {
			for (int x = 0; x < width; ++x) {
				const int offset = bytesPerPixel * (x + y * width);
				pixels[offset + 3] = 255;
			}
		}
		std::string name = file_dialog({ { "png", "png" } }, true);
		if (!name.empty()) {
			name += ".png";
			stbi_write_png(name.c_str(), width, height, 4, pixels, 0);
		}
		delete[] pixels;
	}
}

void Application::LoadMeshDialog() {
	Worker::Do([&]() {
		std::string filename = file_dialog({ { "obj", "Wavefront OBJ" },{ "off", "" } }, false);
		if (!filename.empty()) {
			LoadMesh(filename);
		}
	});
}

void Application::LoadMesh(const std::string filename) {
	mesh().render_ready = false;
	print("Loading %s\n", filename.c_str());

	OpenMesh::IO::Options opt = OpenMesh::IO::Options::VertexNormal;
	if (!OpenMesh::IO::read_mesh(mesh(), filename, opt)) {
		print("Error reading mesh!\n");
		return;
	}
	mesh().initialize();
	if (move_mesh_to_origin) mesh().move_to_origin();
	if (normalize_mesh) mesh().normalize();

	Renderer::Instance().ResetCamera();
	Renderer::Instance().InvalidateGeometry();

	mesh().render_ready = true;

	DoBackup();

	print("Loaded %s\n", filename.c_str());
}
