#include "Renderer.h"

#include <iostream>
#include <assert.h>

#include <imgui.h>
#include <GLFW\glfw3.h>

#include "Log.h"
#include "FileDialog.h"
#include "Converters.h"

namespace {
	Vec3d halfedge_normal(const HalfedgeHandle he) {
		const FaceHandle f1 = cmesh().face_handle(he);
		const HalfedgeHandle opp = cmesh().opposite_halfedge_handle(he);
		const FaceHandle f2 = opp.is_valid() ? cmesh().face_handle(opp) : FaceHandle(-1);
		if (f1.is_valid() && f2.is_valid()) {
			// normal internal edge
			if (cmesh().is_feature(he)) {
				return cmesh().normal(f1);
			} else {
				return (cmesh().normal(f1) + cmesh().normal(f2)) / 2;
			}
		} else if (f1.is_valid()) {
			// boundary edge
			return cmesh().normal(f1);
		} else if (f2.is_valid()) {
			return cmesh().normal(f2);
		} else {
			// free edge
			return Vec3d(0, 0, 0);
		}
	}

	double RayEdgeDist(const Vec3d &rayBase, const Vec3d &rayDirection, const EdgeHandle edge) {
		HalfedgeHandle he = cmesh().halfedge_handle(edge, 0);
		Vec3d start = cmesh().point(cmesh().from_vertex_handle(he));
		Vec3d end = cmesh().point(cmesh().to_vertex_handle(he));

		Vec3d segDirection = end - start;
		Vec3d u = rayBase - start;
		double a = OpenMesh::dot(rayDirection, rayDirection);
		double b = OpenMesh::dot(rayDirection, segDirection);
		double c = OpenMesh::dot(segDirection, segDirection);
		double d = OpenMesh::dot(rayDirection, u);
		double e = OpenMesh::dot(segDirection, u);

		double sNum;
		double tNum;
		double sDenom;
		double tDenom;

		double det = a * c - b * b;

		if (det < EPSILON) {
			sNum = 0;
			tNum = e;
			tDenom = c;
			sDenom = det;
		} else {
			sNum = b * e - c * d;
			tNum = a * e - b * d;
		}

		// check s

		sDenom = det;
		if (sNum < 0) {
			sNum = 0;
			tNum = e;
			tDenom = c;
		} else {
			tDenom = det;
		}
		if (tNum < 0) {
			tNum = 0;
			if (-d < 0) {
				sNum = 0;
			} else {
				sNum = -d;
				sDenom = a;
			}
		} else if (tNum > tDenom) {
			tNum = tDenom;
			if ((-d + b) < 0) {
				sNum = 0;
			} else {
				sNum = -d + b;
				sDenom = a;
			}
		}

		double s = sNum / sDenom;
		double t = tNum / tDenom;
		Vec3d v = (rayBase + s * rayDirection) - (start + t * segDirection);
		return OpenMesh::dot(v, v);
	}

	double RayVertexDist(const Vec3d &rayBase, const Vec3d &rayDirection, const VertexHandle vert) {
		Vec3d p = cmesh().point(vert);
		Vec3d w = (p - rayBase);
		Vec3d c = OpenMesh::cross(rayDirection, w);
		return c.norm();
	}

	double RayPointDist(const Vec3d &rayBase, const Vec3d &rayDirection, const Vec3d  p) {
		Vec3d w = (p - rayBase);
		Vec3d c = OpenMesh::cross(rayDirection, w);
		return c.norm();
	}
}

void Renderer::Terminate() {
	for (int i = 0; i < Render::N_RENDER_MODES; ++i) {
		if (shader[i]) {
			shader[i]->Free();
			delete shader[i];
		}
	}
}

Renderer::Renderer() 
	: fov_(35.0f),
	nearplane_(0.1f),
	farplane_(40.0f),
	width(0),
	height(0),
	
	eye(0.0f, 0.0f, 5.0f),
	center(0.0f, 0.0f, 0.0f),
	up(0.0f, 1.0f, 0.0f),
	arcball(),
	
	updated_geometry_(true) {

	ResetCamera();

	using namespace Converters;

	for (int i = 0; i < Render::N_RENDER_MODES; ++i) {
		active[i] = false;
	}
	active[Shaded] = true;

	// ====== Define shader type ======
	face_shader_list.push_back(Shaded);
	face_shader_list.push_back(Solid);
	face_shader_list.push_back(DebugFace);
	face_shader_list.push_back(PickerFace);

	edge_shader_list.push_back(Wireframe);
	edge_shader_list.push_back(SingleEdge);

	edge_shader_list.push_back(BoundaryEdges);
	edge_shader_list.push_back(FeaturesEdges);
	
	edge_shader_list.push_back(DebugEdge);
	edge_shader_list.push_back(PickerEdge);
	edge_shader_list.push_back(PickerVertex);

	normal_shader_list.push_back(VertexNormals);
	normal_shader_list.push_back(EdgeNormals);
	normal_shader_list.push_back(FaceNormals);

	// ====== Create Shaders ======
	shader[Shaded] = new BasicShader("phong", "phong");

	for (enum Render mode : edge_shader_list) {
		if (shader[mode]) continue;
		shader[mode] = edge_shader[mode] = new EdgeShader("in_color", "in_color", "line");
	}

	for (enum Render mode : halfedge_shader_list) {
		if (shader[mode]) continue;
		shader[mode] = halfedge_shader[mode] = new HalfedgeShader("in_color", "in_color", "line");
	}

	for (enum Render mode : normal_shader_list) {
		if (shader[mode]) continue;
		shader[mode] = custom_shader[mode] = new CustomShader("in_color", "in_color", "normal");
	}

	for (enum Render mode : face_shader_list) {
		if (shader[mode]) continue;
		shader[mode] = face_shader[mode] = new FaceShader("in_color", "in_color", "triangle");
	}

	// ====== Set Render function ======
	for (enum Render mode : face_shader_list) {
		BasicShader* s = shader[mode];
		s->SetRenderFunc([s, this]() {
			glDepthMask(GL_TRUE);
			s->Bind();
			s->SetUniform("modelViewProjMatrix", modelViewProj);
			s->DrawArray();
		});
	}

	for (std::vector<enum Render> render_list :
	{ edge_shader_list, halfedge_shader_list, normal_shader_list }) {
		for (enum Render mode : render_list) {
			BasicShader* s = shader[mode];
			s->SetRenderFunc([s, this]() {
				glDepthMask(GL_FALSE);
				if (line_depth_test) {
					glEnable(GL_DEPTH_TEST);
				} else {
					glDisable(GL_DEPTH_TEST);
				}
				s->Bind();
				s->SetUniform("modelViewProjMatrix", modelViewProj);
				s->DrawArray();
			});
		}
	}

	shader[Shaded]->SetRenderFunc([this]() {
		glDepthMask(GL_TRUE);
		shader[Shaded]->Bind();
		shader[Shaded]->SetUniform("modelViewProjMatrix", modelViewProj);
		shader[Shaded]->SetUniform("modelViewMatrix", modelView);
		shader[Shaded]->SetUniform("normalMatrix", normalmatrix);
		shader[Shaded]->SetUniform("viewMatrix", glm::mat3(view));
		shader[Shaded]->SetUniform("l0Position", l0Position);
		shader[Shaded]->SetUniform("l1Position", l1Position);
		shader[Shaded]->SetUniform("l2Position", l2Position);
		shader[Shaded]->SetUniform("DiffuseMaterial", DiffuseMaterial);
		shader[Shaded]->SetUniform("AmbientMaterial", AmbientMaterial);
		shader[Shaded]->SetUniform("SpecularMaterial", SpecularMaterial);
		shader[Shaded]->SetUniform("l0Intensity", l0Intensity);
		shader[Shaded]->SetUniform("l1Intensity", l1Intensity);
		shader[Shaded]->SetUniform("l2Intensity", l2Intensity);
		shader[Shaded]->SetUniform("ambient_intensity", phong_ambient_intensity);
		shader[Shaded]->SetUniform("diffuse_intensity", phong_diffuse_intensity);
		shader[Shaded]->SetUniform("specular_intensity", phong_specular_intensity);
		shader[Shaded]->SetUniform("shininess", phong_specular_shininess);
		shader[Shaded]->DrawArray();
	});

	shader[Solid]->SetRenderFunc([this]() {
		glDepthMask(GL_TRUE);
		//glDepthFunc(GL_ALWAYS);
		shader[Solid]->Bind();
		shader[Solid]->SetUniform("modelViewProjMatrix", modelViewProj);
		shader[Solid]->DrawArray();
		//glDepthFunc(GL_LEQUAL);
	});

	// ====== Set Color Function ======
	SetWireframeColor();
	SetSolidColor();
	SetBoundaryColor();
	SetFeatureColor();

	edge_shader[SingleEdge]->SetColorFunc(
		[&](EdgeHandle e, unsigned int) -> Color {
		for (int i = 0; i < selected_edge_id.size(); ++i) {
			if (e.idx() == selected_edge_id[i]) {
				ImColor c = selected_edge_color[i];
				return Converters::convert(c);
			}
		}
		return Color::Empty();
	});

	custom_shader[VertexNormals]->SetColorFunc([]() {
		Eigen::Matrix4Xf colors(4, cmesh().n_vertices() * 2);
		size_t i = 0;
		for (VertexHandle v : cmesh().vertices()) {
			Vec3d n = cmesh().normal(v);
			colors.col(i) = colors.col(i + 1) = normal2color(n);
			i += 2;
		}
		return colors;
	});
	custom_shader[EdgeNormals]->SetColorFunc([]() {
		Eigen::Matrix4Xf colors(4, cmesh().n_halfedges() * 2);
		size_t i = 0;
		for (const HalfedgeHandle he : cmesh().halfedges()) {
			const Vec3d n = halfedge_normal(he);
			colors.col(i) = colors.col(i + 1) = normal2color(n);
			i += 2;
		}
		return colors;
	});
	custom_shader[FaceNormals]->SetColorFunc([]() {
		Eigen::Matrix4Xf colors(4, cmesh().n_faces() * 2);
		size_t i = 0;
		for (const FaceHandle f : cmesh().faces()) {
			const Vec3d n = cmesh().normal(f);
			colors.col(i) = colors.col(i + 1) = normal2color(n);
			i += 2;
		}
		return colors;
	});

	edge_shader[DebugEdge]->SetColorFunc(
		[this](const EdgeHandle e, const unsigned int i) -> Color {

		return Color::Red();
	});

	face_shader[DebugFace]->SetColorFunc(
		[this](const FaceHandle f, const VertexHandle) -> Color {
		return Color::Red();
	});

	face_shader[PickerFace]->SetColorFunc(
		[this](const FaceHandle f, const VertexHandle) -> Color {
		if (f == picked_face) return Color::Red();
		return Color::Empty();
	});
	edge_shader[PickerEdge]->SetColorFunc(
		[this](const EdgeHandle e, const unsigned int) -> Color {
		if (e == picked_edge) return Color::Red();
		return Color::Empty();
	});
	edge_shader[PickerVertex]->SetColorFunc(
		[this](const EdgeHandle e, const unsigned int i) -> Color {
		const HalfedgeHandle he = cmesh().halfedge_handle(e, i);
		if (cmesh().from_vertex_handle(he) == picked_vertex) {
			return Color::Red();
		}
		return Color::Empty();
	});
}

void Renderer::UploadMeshData() {
	using namespace Converters;

	mesh().update_normals();

	float average_edge_length = (float)mesh().calc_average_edge_length();
	double normal_length_factor = average_edge_length * 0.8;

	size_t i;
	GLuint n_vertices = static_cast<GLuint>(cmesh().n_vertices());
	GLuint n_faces = static_cast<GLuint>(cmesh().n_faces());
	GLuint n_edges = static_cast<GLuint>(cmesh().n_edges());
	GLuint n_halfedges = static_cast<GLuint>(cmesh().n_halfedges());
	GLuint n_triangles = 0;
	for (FaceHandle f : cmesh().faces()) {
		GLuint face_triangles = cmesh().valence(f) - 2;
		n_triangles += face_triangles;
	}
	assert(n_triangles >= n_faces);
	
	i = 0; // triangle vertex index
	Eigen::Matrix3Xf face_vertices(3, n_triangles * 3);
	Eigen::Matrix3Xf face_normals(3, n_triangles * 3);
	for (FaceHandle f : cmesh().faces()) {
		// this is basically a triangle fan for any face valence
		MyMesh::ConstFaceVertexCCWIter it = cmesh().cfv_ccwbegin(f);
		VertexHandle first = *it;
		++it;
		size_t face_triangles = cmesh().valence(f) - 2;
		for (int j = 0; j < face_triangles; ++j) {
			face_vertices.col(i + 0) = d2f(cmesh().point(first));
			face_normals.col(i + 0) = d2f(cmesh().good_normal(first, f));

			face_vertices.col(i + 1) = d2f(cmesh().point(*it));
			face_normals.col(i + 1) = d2f(cmesh().good_normal(*it, f));
			++it;
			face_vertices.col(i + 2) = d2f(cmesh().point(*it));
			face_normals.col(i + 2) = d2f(cmesh().good_normal(*it, f));
			i += 3;
		}
	}
	shader[Shaded]->Bind();
	shader[Shaded]->UploadAttrib("position", face_vertices);
	shader[Shaded]->UploadAttrib("normal", face_normals);
	shader[Shaded]->SetPrimitives(GL_TRIANGLES, n_triangles * 3);
	for (enum Render mode : face_shader_list) {
		if (shader[mode]) {
			shader[mode]->Bind();
			shader[mode]->ShareAttrib(*shader[Shaded], "position");
			shader[mode]->SetPrimitives(GL_TRIANGLES, n_triangles * 3);
		}
	}
	assert(i == n_triangles * 3);

	i = 0; // edge index;
	Eigen::Matrix3Xf edge_vertices(3, n_edges * 2);
	Eigen::Matrix3Xf edge_normals(3, n_edges * 2);
	for (EdgeHandle e : cmesh().edges()) {
		HalfedgeHandle he = cmesh().halfedge_handle(e, 0);
		edge_vertices.col(i + 0) = d2f(cmesh().point(cmesh().from_vertex_handle(he)));
		edge_vertices.col(i + 1) = d2f(cmesh().point(cmesh().to_vertex_handle(he)));
		edge_normals.col(i + 0) = d2f(cmesh().normal(e));
		edge_normals.col(i + 1) = d2f(cmesh().normal(e));
		i += 2;
	}
	shader[Wireframe]->Bind();
	shader[Wireframe]->UploadAttrib("position", edge_vertices);
	shader[Wireframe]->UploadAttrib("normal", edge_normals);
	shader[Wireframe]->SetPrimitives(GL_LINES, n_edges * 2);
	for (enum Render mode : edge_shader_list) {
		if (shader[mode]) {
			shader[mode]->Bind();
			shader[mode]->ShareAttrib(*shader[Wireframe], "position");
			shader[mode]->ShareAttrib(*shader[Wireframe], "normal");
			shader[mode]->SetPrimitives(GL_LINES, n_edges * 2);
		}
	}
	assert(i == n_edges * 2);
	
	//i = 0; // halfedge index
	//Eigen::Matrix3Xf halfedge_vertices(3, n_halfedges * 2);
	//Eigen::Matrix3Xf halfedge_normals(3, n_halfedges * 2);
	//for (HalfedgeHandle he : cmesh().halfedges()) {
	//	FaceHandle f = cmesh().face_handle(he);
	//	if (!f.is_valid()) { // use opposite face as fallback
	//		HalfedgeHandle opp = cmesh().opposite_halfedge_handle(he);
	//		f = cmesh().face_handle(opp);
	//	}
	//	Vector3f normal = convert(f.is_valid() ? cmesh().normal(f) : cmesh().normal(he));
	//	Vector3f vector = convert(cmesh().vector(he));
	//	normal.normalize();
	//	vector.normalize();
	//	Vector3f left = normal.cross(vector);
	//	Vector3f from = convert(cmesh().point(cmesh().from_vertex_handle(he)));
	//	Vector3f to = convert(cmesh().point(cmesh().to_vertex_handle(he)));
	//	halfedge_vertices.col(i + 0) = from + left * halfedge_side_offset;
	//	halfedge_vertices.col(i + 1) = to + left * halfedge_side_offset;
	//	halfedge_normals.col(i + 0) = normal;
	//	halfedge_normals.col(i + 1) = normal;
	//	i += 2;
	//}
	//assert(i == n_halfedges * 2);

	i = 0; // vert normal index
	Eigen::Matrix3Xf vertnormal_vertices(3, n_vertices * 2);
	for (VertexHandle v : cmesh().vertices()) {
		vertnormal_vertices.col(i + 0) = d2f(cmesh().point(v));
		vertnormal_vertices.col(i + 1) = d2f(cmesh().point(v) + cmesh().normal(v) * normal_length_factor);
		i += 2;
	}
	shader[VertexNormals]->Bind();
	shader[VertexNormals]->UploadAttrib("position", vertnormal_vertices);
	shader[VertexNormals]->SetPrimitives(GL_LINES, n_vertices * 2);
	assert(i == n_vertices * 2);

	i = 0; // halfedge normal index
	Eigen::Matrix3Xf halfedgenormal_vertices(3, n_halfedges * 2);
	for (HalfedgeHandle he : cmesh().halfedges()) {
		Vec3d normal = halfedge_normal(he);
		Vec3d from = cmesh().midpoint(he);
		normal *= normal_length_factor;
		halfedgenormal_vertices.col(i + 0) = d2f(from);
		halfedgenormal_vertices.col(i + 1) = d2f(from + normal);
		i += 2;
	}
	shader[EdgeNormals]->Bind();
	shader[EdgeNormals]->UploadAttrib("position", halfedgenormal_vertices);
	shader[EdgeNormals]->SetPrimitives(GL_LINES, n_halfedges * 2);
	assert(i == n_halfedges * 2);

	i = 0;
	Eigen::Matrix3Xf facenormal_vertices(3, n_faces * 2);
	for (FaceHandle f : cmesh().faces()) {
		facenormal_vertices.col(i + 0) = d2f(cmesh().midpoint(f));
		Vec3d normal = cmesh().normal(f) * normal_length_factor;
		facenormal_vertices.col(i + 1) = facenormal_vertices.col(i) + d2f(normal);
		i += 2;
	}
	shader[FaceNormals]->Bind();
	shader[FaceNormals]->UploadAttrib("position", facenormal_vertices);
	shader[FaceNormals]->SetPrimitives(GL_LINES, n_faces * 2);
	assert(i == n_faces * 2);

	UpdateLineThickness();
	//UpdateLineBump();

	print("Uploading mesh data\n");
	print("n_vertices: %lu\n", n_vertices);
	print("n_faces: %lu\n", n_faces);
	print("n_triangles: %lu\n", n_triangles);
	print("n_edges: %lu\n", n_edges);
	print("n_halfedges: %lu\n", n_halfedges);
}

void Renderer::UpdateLineThickness() {
	float line = (float)mesh().average_edge_length() * line_thickness_factor;
	float wire = (float)mesh().average_edge_length() * wire_thickness_factor;
	for (enum Render mode : edge_shader_list) {
		if (shader[mode]) {
			shader[mode]->Bind();
			shader[mode]->SetUniform("thickness", line);
		}
	}
	shader[Wireframe]->Bind();
	shader[Wireframe]->SetUniform("thickness", wire);
	shader[VertexNormals]->Bind();
	shader[VertexNormals]->SetUniform("thickness", line);
	shader[EdgeNormals]->Bind();
	shader[EdgeNormals]->SetUniform("thickness", line);
	shader[FaceNormals]->Bind();
	shader[FaceNormals]->SetUniform("thickness", line);
}

void Renderer::Render() {
	if (width == 0 || height == 0) return;

	if (cmesh().render_ready) {
		if (!updated_geometry_) {
			UploadMeshData();

			updated_geometry_ = true;
			for (int i = 0; i < Render::N_RENDER_MODES; ++i) {
				if (shader[i]) {
					shader[i]->Invalidate();
				}
			}
		}

		if (cmesh().n_vertices() > 0) {
			for (int i = 0; i < Render::N_RENDER_MODES; ++i) {
				if (shader[i] && active[i]) {
					shader[i]->Update();
				}
			}
		}
	}

	glDepthMask(GL_TRUE);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);
	glClear(GL_DEPTH_BUFFER_BIT);
	glEnable(GL_BLEND);
	glEnable(GL_MULTISAMPLE);
	glBlendEquationSeparate(GL_FUNC_ADD, GL_FUNC_ADD);
	glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ZERO);

	Color clear_color(1.0f, 1.0f, 1.0f);
	glViewport(0, 0, width, height);
	glClearColor(clear_color.r(), clear_color.g(), clear_color.b(), clear_color.a());
	glClear(GL_COLOR_BUFFER_BIT);

	view = glm::lookAt(eye, center, up);
	projection = glm::perspective(glm::radians(fov_), 
		(float)width / (float)height, nearplane_, farplane_);

	Matrix4f m = arcball.matrix();
	model = glm::mat4(m(0), m(1), m(2), m(3),
		m(4), m(5), m(6), m(7),
		m(8), m(9), m(10), m(11),
		m(12), m(13), m(14), m(15));
	model = glm::scale(model, glm::vec3(scale_, scale_, scale_));
	model = glm::translate(model, translate_);
	model = glm::rotate(model, Utils::ToRad(rotation_amount), rotation_axis);
	model = glm::translate(model, rotation_center);


	modelViewProj = projection * view * model;
	modelView = view * model;
	normalmatrix = glm::inverseTranspose(glm::mat3(modelView));

	// if Solid color mesh render is semi-transparent, then we need to render everything twice
	// basically it works like this:
	// 1. first we render all chains, depth buffer is empty so they'll be rendered everywhere
	// 2. then we render the mesh, writing on the depth buffer but not checking for it. 
	//    this will make the chains in the back look in the back (as we are writing over them)
	//    and the whole mesh will be rendered, so we don't care about polygon order
	// 3. finally we render all chains again. Depth test is enabled now and the mesh wrote 
	//	  to the z-buffer, so back chains won't be rendered. This pass properly renders the chains
	//    in the front. 
	// Issues: 
	// - everything is rendered more or less twice, so the actual final color
	//	 of opaque elements is not what it is supposed to be
	// - sometimes some back chains are rendered as if they were in the
	//   front, maybe the z-buffer fails on the second pass, or the mesh doesn't 
	//   write there, or it's completely different reason I cannot figure out
	//if (active[Solid] && solidcolor.Value.w < 1.0f && !active[Shaded] && !active[Solid]) {
	//	for (unsigned i = Render::Regions; i < Render::N_RENDER_MODES; ++i) {
	//		if (active[i] && shader[i]) {
	//			shader[i]->Render();
	//		}
	//	}
	//	shader[Solid]->Render();
	//	for (unsigned i = Render::Regions; i < Render::N_RENDER_MODES; ++i) {
	//		if (active[i] && shader[i]) {
	//			shader[i]->Render();
	//		}
	//	}
	//} else {
	for (unsigned i = 0; i < Render::N_RENDER_MODES; ++i) {
		if (active[i] && shader[i]) {
			if (normal_lines) {
				shader[i]->SetUniform("flip_lines", false);
				shader[i]->Render();
			}
			if (flipped_lines) {
				shader[i]->SetUniform("flip_lines", true);
				shader[i]->Render();
			}
		}
	}
	//}
}

void Renderer::cursorpos_callback(GLFWwindow* win, double x, double y) {
	moved_since_mouse_press = true;

	if (rotate_active_ && !lock_camera) {
		arcball.motion(Vector2i(x, y));
	} else if (translate_xy_active_ && !lock_camera) {
		glm::vec3 up = glm::vec3(glm::inverse(modelView) * glm::vec4(0.0f, 1.0f, 0.0f, 0.0f));
		glm::vec3 right = glm::vec3(glm::inverse(modelView) * glm::vec4(1.0f, 0.0f, 0.0f, 0.0f));
		
		glm::vec2 curr(x, y);
		glm::vec2 diff = curr - translate_start_;
		translate_ += translate_speed_ * right * (diff.x);
		translate_ += translate_speed_ * up * (-diff.y);
		translate_start_ = curr;
	} else if (translate_z_active_ && !lock_camera) {
		glm::vec3 look = glm::vec3(glm::inverse(modelView) * glm::vec4(0.0f, 0.0f, 1.0f, 0.0f));
		glm::vec2 curr(x, y);
		glm::vec2 diff = curr - translate_start_;
		translate_ += translate_speed_ * look * (-diff.y);
		translate_start_ = curr;
	} else if (active[PickerVertex] || active[PickerEdge] || active[PickerFace]) {
		// line selection mode
		int vp[4];
		glGetIntegerv(GL_VIEWPORT, vp);
		glm::uvec4 viewport(vp[0], vp[1], vp[2], vp[3]);
		glm::vec3 rayO = glm::unProject({ x, height - y, 0 }, modelView, projection, viewport);
		glm::vec3 rayD = glm::unProject({ x, height - y, 1 }, modelView, projection, viewport);
		glm::vec3 ray = rayD - rayO;
		Vec3d rayOrigin(rayO[0], rayO[1], rayO[2]);
		Vec3d rayDirection(ray[0], ray[1], ray[2]);

		{ // edge picker
			double bestDistance = 100000000000.0;
			EdgeHandle new_selected_edge;
			for (const EdgeHandle e : cmesh().edges()) {
				double distance = RayEdgeDist(rayOrigin, rayDirection, e);
				//glm::vec3 proj = glm::project({ mid[0], mid[1], mid[2] }, modelView, projection, viewport);
				//distance to camera is proj.z
				double c = Utils::CosAngleBetween(rayDirection, cmesh().normal(e));
				// get closer to the ray
				if (c < 0 && distance < bestDistance) {
					bestDistance = distance;
					new_selected_edge = e;
				}
			}
			if (new_selected_edge.is_valid() && new_selected_edge != picked_edge) {
				picked_edge = new_selected_edge;
				GetShader(PickerEdge)->Invalidate();
			}
		}

		{ // vertex picker
			double bestDistance = 100000000000.0;
			VertexHandle new_selected_vertex;
			for (const VertexHandle v : cmesh().vertices()) {
				double distance = RayVertexDist(rayOrigin, rayDirection, v);
				double c = Utils::CosAngleBetween(rayDirection, cmesh().normal(v));
				if (c < 0 && distance < bestDistance) {
					bestDistance = distance;
					new_selected_vertex = v;
				}
			}
			if (new_selected_vertex.is_valid() && new_selected_vertex != picked_vertex) {
				picked_vertex = new_selected_vertex;
				GetShader(PickerVertex)->Invalidate();
			}
		}

		{ // face picker
			// I'm lazy so I just use distance to center of the face
			double bestDistance = 100000000000.0;
			FaceHandle new_selected_face;
			for (const FaceHandle f : cmesh().faces()) {
				double distance = RayPointDist(rayOrigin, rayDirection, cmesh().midpoint(f));
				double c = Utils::CosAngleBetween(rayDirection, cmesh().normal(f));
				if (c < 0 && distance < bestDistance) {
					bestDistance = distance;
					new_selected_face = f;
				}
			}
			if (new_selected_face.is_valid() && new_selected_face != picked_vertex) {
				picked_face = new_selected_face;
				GetShader(PickerFace)->Invalidate();
			}
		}
	}
}

void Renderer::mouseButton_callback(GLFWwindow* win, int button, int action, int modifiers) {
	double x, y;
	glfwGetCursorPos(win, &x, &y);
	
	if (button == GLFW_MOUSE_BUTTON_LEFT) {
		rotate_active_ = (action == GLFW_PRESS);
		arcball.button(Vector2i(x, y), rotate_active_);

		if (action == GLFW_PRESS) {
			moved_since_mouse_press = false;
		} else if (action == GLFW_RELEASE && !moved_since_mouse_press) {
			if (active[PickerEdge] && picked_edge.is_valid()) {
				print("\n === Selected Edge handle %d === \n", picked_edge.idx());
			} else if (active[PickerVertex] && picked_vertex.is_valid()) {
				print("\n === Selected Vertex handle %d === \n", picked_edge.idx());
			} else if (active[PickerFace] && picked_face.is_valid()) {
				print("\n === Selected Face handle %d === \n", picked_face.idx());
			}
		}
	} else if (button == GLFW_MOUSE_BUTTON_RIGHT) {
		translate_xy_active_ = (action == GLFW_PRESS);
		translate_start_ = glm::vec2(x, y);
	} else if (button == GLFW_MOUSE_BUTTON_MIDDLE) {
		translate_z_active_ = (action == GLFW_PRESS);
		translate_start_ = glm::vec2(x, y);
	}
}

void Renderer::scroll_callback(GLFWwindow* win, double x_offset, double y_offset) {
	if (lock_camera) return;
	if (y_offset > 0) {
		scale_ *= 1.25f;
	} else {
		scale_ *= 0.8f;
	}
}

void Renderer::Resize(int w, int h) {
	width = w;
	height = h;
	arcball.setSize(Vector2i(w, h));
}

void Renderer::ResetCamera() {
	if (lock_camera) return;
	translate_ = glm::vec3(0, 0, 0);
	arcball = Arcball();
	arcball.setSize(Vector2i(width, height));
	rotate_active_ = false;
	translate_xy_active_ = false;
	translate_z_active_ = false;
	scale_ = 4.0f;
}

void Renderer::SetSolidColor() {
	face_shader[Solid]->SetColorFunc(
		[this](const FaceHandle, const VertexHandle) {
		return Converters::convert(solidcolor);
	});
}

void Renderer::SetWireframeColor() {
	edge_shader[Wireframe]->SetColorFunc(
		[&](EdgeHandle, unsigned int) -> Color {
		return Converters::convert(wirecolor);
	});
}

void Renderer::SetBoundaryColor() {
	edge_shader[BoundaryEdges]->SetColorFunc(
		[this](const EdgeHandle e, unsigned int) -> Color {
		if (cmesh().is_boundary(e)) {
			return Converters::convert(boundarycolor);
		} else {
			return Color::Empty();
		}
	});
}

void Renderer::SetFeatureColor() {
	edge_shader[FeaturesEdges]->SetColorFunc(
		[this](const EdgeHandle e, unsigned int) -> Color {
		if (cmesh().is_feature(e)) {
			return Converters::convert(featurecolor);
		} else {
			return Color::Empty();
		}
	});
}

void Renderer::DrawInterface(bool *p_open) {
	ImGui::Begin("Visualization", p_open);
	ImGui::Checkbox("Mesh: Shaded", &active[Shaded]);
	ImGui::SameLine();
	if (ImGui::TreeNode("Params##phong")) {
		ImGui::DragFloat("Ambient", &phong_ambient_intensity, 0.01f, 0.0f, 1.0f);
		ImGui::DragFloat("Diffuse", &phong_diffuse_intensity, 0.01f, 0.0f, 1.0f);
		ImGui::DragFloat("Specular", &phong_specular_intensity, 0.01f, 0.0f, 1.0f);
		ImGui::DragFloat("Shininess", &phong_specular_shininess, 0.1f, 0.5f, 20.0f);
		ImGui::DragFloat3("l0Position", glm::value_ptr(l0Position), 0.01f);
		ImGui::DragFloat3("l1Position", glm::value_ptr(l1Position), 0.01f);
		ImGui::DragFloat3("l2Position", glm::value_ptr(l2Position), 0.01f);
		ImGui::DragFloat("l0Intensity", &l0Intensity, 0.01f, 0.0f, 1.0f);
		ImGui::DragFloat("l1Intensity", &l1Intensity, 0.01f, 0.0f, 1.0f);
		ImGui::DragFloat("l2Intensity", &l2Intensity, 0.01f, 0.0f, 1.0f);
		ImGui::DragFloat3("DiffuseMaterial", glm::value_ptr(DiffuseMaterial), 0.01f, 0.0f, 1.0f);
		ImGui::DragFloat3("AmbientMaterial", glm::value_ptr(AmbientMaterial), 0.01f, 0.0f, 1.0f);
		ImGui::DragFloat3("SpecularMaterial", glm::value_ptr(SpecularMaterial), 0.01f, 0.0f, 1.0f);
		ImGui::TreePop();
	}
	ImGui::Checkbox("Mesh: Solid", &active[Solid]);
	ImGui::SameLine();
	ImGui::ColorButton("", solidcolor);
	if (ImGui::BeginPopupContextItem("solidcolor popup")) {
		ImGui::Text("Solid Color");
		if (ImGui::ColorEdit4("##solidcolor", (float*)&solidcolor)) {
			SetSolidColor();
		}
		ImGui::EndPopup();
	}

	ImGui::Checkbox("Wireframe", &active[Wireframe]);
	ImGui::SameLine();
	ImGui::ColorButton("", wirecolor);
	if (ImGui::BeginPopupContextItem("wirecolor popup")) {
		ImGui::Text("Wireframe Color");
		if (ImGui::ColorEdit4("##wirecolor", (float*)&wirecolor)) {
			SetWireframeColor();
		}
		ImGui::EndPopup();
	}
	ImGui::Checkbox("Single Edge", &active[SingleEdge]);
	ImGui::SameLine();
	if (ImGui::TreeNode("Params##single_edge")) {
		ImGui::Unindent();
		for (int i = 0; i < selected_edge_id.size(); ++i) {
			ImGui::ColorButton("", selected_edge_color[i]);
			if (ImGui::BeginPopupContextItem("selected edge color")) {
				if (ImGui::ColorEdit4(std::to_string(i).c_str(), (float*)&selected_edge_color[i])) {
					GetShader(SingleEdge)->Invalidate();
				}
				ImGui::EndPopup();
			}
			ImGui::SameLine();
			if (ImGui::InputInt(std::to_string(i).c_str(), &selected_edge_id[i])) {
				GetShader(SingleEdge)->Invalidate();
			}
		}
		if (ImGui::Button("Add Edge")) {
			selected_edge_id.push_back(-1);
			selected_edge_color.push_back(Converters::convert(Color::Random()));
		}
		ImGui::SameLine();
		if (ImGui::Button("Remove Edge") && !selected_edge_id.empty()) {
			selected_edge_id.pop_back();
			selected_edge_color.pop_back();
		}
		ImGui::Indent();
		ImGui::TreePop();
	}
	ImGui::Checkbox("Feature Edges", &active[FeaturesEdges]);
	ImGui::SameLine();
	ImGui::ColorButton("", featurecolor);
	if (ImGui::BeginPopupContextItem("featurecolor popup")) {
		ImGui::Text("Feature Color");
		if (ImGui::ColorEdit4("##featurecolor", (float*)&featurecolor)) {
			SetFeatureColor();
		}
		ImGui::EndPopup();
	}

	ImGui::Checkbox("Boundary Edges", &active[BoundaryEdges]);
	ImGui::SameLine();
	ImGui::ColorButton("", boundarycolor);
	if (ImGui::BeginPopupContextItem("boundarycolor popup")) {
		ImGui::Text("Boundary Color");
		if (ImGui::ColorEdit4("##boundarycolor", (float*)&boundarycolor)) {
			SetBoundaryColor();
		}
		ImGui::EndPopup();
	}
	if (ImGui::TreeNode("Line Params")) {
		if (ImGui::DragFloat("Wire##thickness", &wire_thickness_factor, 0.001f, 0.0f, 0.5f)) {
			UpdateLineThickness();
		}
		if (ImGui::DragFloat("Lines##thickness", &line_thickness_factor, 0.001f, 0.0f, 0.5f)) {
			UpdateLineThickness();
		}
		ImGui::Checkbox("Normal Lines", &normal_lines);
		ImGui::Checkbox("Flipped Lines", &flipped_lines);
		ImGui::Checkbox("Depth Test", &line_depth_test);
		ImGui::TreePop();
	}

	ImGui::Separator();
	
	ImGui::Checkbox("Debug", &active[DebugEdge]);
	ImGui::Checkbox("Debug Face", &active[DebugFace]);
	ImGui::Checkbox("Picker (Vertex)", &active[PickerVertex]);
	ImGui::Checkbox("Picker (Edge)", &active[PickerEdge]);
	ImGui::Checkbox("Picker (Face)", &active[PickerFace]);
	ImGui::Separator();
	ImGui::Checkbox("Vertex Normals", &active[VertexNormals]);
	ImGui::Checkbox("Edge Normals", &active[EdgeNormals]);
	ImGui::Checkbox("Face Normals", &active[FaceNormals]);
	if (ImGui::Button("Reset Camera", ImVec2(150, 0))) {
		ResetCamera();
	}
	if (ImGui::Button("Invalidate All", ImVec2(150, 0))) {
		InvalidateGeometry();
	}
	if (ImGui::IsItemHovered()) ImGui::SetTooltip("Force refresh everything");
	ImGui::End();
}

void Renderer::DrawCameraInterface(bool* p_open) {
	ImGui::Begin("Camera", p_open);
	ImGui::Checkbox("Lock Camera", &lock_camera);
	ImGui::Separator();
	ImGui::DragFloat3("eye", glm::value_ptr(eye), 0.01f);
	ImGui::DragFloat3("center", glm::value_ptr(center), 0.01f);
	ImGui::DragFloat3("Up", glm::value_ptr(up), 0.01f);
	ImGui::Text("View matrix");
	ImGui::DragFloat4("###renderdragfloat1", glm::value_ptr(view) + 0, 0.1f);
	ImGui::DragFloat4("###renderdragfloat2", glm::value_ptr(view) + 4, 0.1f);
	ImGui::DragFloat4("###renderdragfloat3", glm::value_ptr(view) + 8, 0.1f);
	ImGui::DragFloat4("###renderdragfloat4", glm::value_ptr(view) +12, 0.1f);
	if (ImGui::SmallButton("reset viewmatrix")) {
		eye = glm::vec3(0.0f, 0.0f, 5.0f);
		center = glm::vec3(0.0f, 0.0f, 0.0f);
		up = glm::vec3(0.0f, 1.0f, 0.0f);
	}
	ImGui::Separator();
	ImGui::DragFloat("Scale", &scale_, 0.01f);
	ImGui::DragFloat4("Rotation", (float*)&(arcball.state()), 0.01f, 0.0f, 1.0f);
	ImGui::DragFloat3("Translate", glm::value_ptr(translate_), 0.01f);
	ImGui::SliderFloat("Translation speed", &translate_speed_, 0.0f, 0.03f);
	ImGui::Text("Turntable:");
	ImGui::DragFloat("Angle", &rotation_amount, 1.0f, 0.0f, 360.0f);
	ImGui::DragFloat3("Center", glm::value_ptr(rotation_center), 0.01f);
	ImGui::DragFloat3("Axis", glm::value_ptr(rotation_axis), 0.01f);
	ImGui::Text("Model matrix");
	ImGui::DragFloat4("###renderdragfloat5",glm::value_ptr(model) + 0, 0.1f);
	ImGui::DragFloat4("###renderdragfloat6",glm::value_ptr(model) + 4, 0.1f);
	ImGui::DragFloat4("###renderdragfloat7",glm::value_ptr(model) + 8, 0.1f);
	ImGui::DragFloat4("###renderdragfloat8",glm::value_ptr(model) +12, 0.1f);
	if (ImGui::SmallButton("reset model transforms")) {
		arcball.setState(Eigen::Quaternionf(1.0f, 0.0f, 0.0f, 0.0f));
		translate_ = glm::vec3(0.0f, 0.0f, 0.0f);
		scale_ = 1.0f;
	}
	ImGui::Separator();
	ImGui::Text("Projection matrix");
	ImGui::DragFloat("fov", &fov_);
	ImGui::DragFloat("near plane", &nearplane_);
	ImGui::DragFloat("far plane", &farplane_);
	ImGui::DragInt("width", &width);
	ImGui::DragInt("height", &height);
	ImGui::DragFloat4("###renderdragfloat9", glm::value_ptr(projection) +  0, 0.1f);
	ImGui::DragFloat4("###renderdragfloat10", glm::value_ptr(projection) + 4, 0.1f);
	ImGui::DragFloat4("###renderdragfloat11", glm::value_ptr(projection) + 8, 0.1f);
	ImGui::DragFloat4("###renderdragfloat12", glm::value_ptr(projection) +12, 0.1f);
	ImGui::End();
}
