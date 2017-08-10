#pragma once

#include <string>
#include <functional>

#include "GLShader.h"
#include "Color.h"
#include "MyMesh.h"

class BasicShader : public GLShader {
public:
	BasicShader(const std::string vert, 
		const std::string frag, 
		const std::string geom) :
		render_([]() {}) {

		const std::string folder = "../shaders/";
		const std::string vert_file = folder + vert + ".vert";
		const std::string frag_file = folder + frag + ".frag";
		const std::string geom_file = folder + geom + ".geom";
		InitFromFiles(vert_file, frag_file, geom_file);
		updated_ = true;
	}
	BasicShader(const std::string vert, 
		const std::string frag)
		: BasicShader(vert, frag, "") {}

	void SetRenderFunc(const std::function<void()> render) { render_ = render; };
	void Render() { render_(); }

	void Invalidate() { updated_ = false; }
	virtual void Update() { updated_ = true; }

protected:
	bool updated_;
	std::function<void()> render_;
};

class FaceShader : public BasicShader {
public:
	FaceShader(const std::string vert,
		const std::string frag, 
		const std::string geom)
		: BasicShader(vert, frag, geom),
		face_color_([](const FaceHandle f, const VertexHandle v) { return Color::Empty(); }) {}
	FaceShader(const std::string vert, 
		const std::string frag)
		: FaceShader(vert, frag, "") {}
	
	void Update() override {
		if (updated_) return;
		updated_ = true;
		size_t i = 0; // triangle vertex index;
		size_t n_triangles = 0;
		for (const FaceHandle f : cmesh().faces()) {
			const size_t face_triangles = cmesh().valence(f) - 2;
			n_triangles += face_triangles;
		}
		Eigen::Matrix4Xf face_colors(4, n_triangles * 3);
		for (const FaceHandle f : cmesh().faces()) {
			const size_t face_triangles = cmesh().valence(f) - 2;
			//// vertex colors version:
			//MyMesh::ConstFaceVertexCCWIter it = cmesh().cfv_ccwbegin(f);
			//VertexHandle first = *it;
			//++it;
			//for (int j = 0; j < face_triangles; ++j) {
			//	face_colors.col(i + 0) = face_color_(f, first);
			//	face_colors.col(i + 1) = face_color_(f, *it);
			//	++it;
			//	face_colors.col(i + 2) = face_color_(f, *it);
			//	i += 3;
			//}

			// uniform color version
			Color color = face_color_(f, VertexHandle(-1));
			for (size_t j = 0; j < face_triangles * 3; ++j) {
				face_colors.col(i + j) = color;
			}
			i += 3 * face_triangles;
		}
		assert(i == n_triangles * 3);
		Bind();
		UploadAttrib("vert_color", face_colors);
	}

	void SetColorFunc(const std::function<Color(const FaceHandle f, const VertexHandle v)> f) {
		face_color_ = f;
		Invalidate();
	}

private:
	std::function<Color(const FaceHandle f, const VertexHandle v)> face_color_;
};

class EdgeShader : public BasicShader {
public:
	EdgeShader(const std::string vert,
		const std::string frag, 
		const std::string geom)
		: BasicShader(vert, frag, geom), 
		edge_color_([](const EdgeHandle, const unsigned int direction) { return Color::Empty(); }) {}
	EdgeShader(const std::string vert, 
		const std::string frag)
		: EdgeShader(vert, frag, "") {}

	void Update() override {
		if (updated_) return;
		updated_ = true;
		size_t i = 0; // line index;
		Eigen::Matrix4Xf line_colors(4, cmesh().n_edges() * 2);
		for (const EdgeHandle e : cmesh().edges()) {
			line_colors.col(i + 0) = edge_color_(e, 0);
			line_colors.col(i + 1) = edge_color_(e, 1);
			i += 2;
		}
		assert(i == cmesh().n_edges() * 2);
		Bind();
		UploadAttrib("vert_color", line_colors);
	}

	void SetColorFunc(std::function<Color(const EdgeHandle, const unsigned int direction)> f) {
		edge_color_ = f;
		Invalidate();
	}

private:
	std::function<Color(const EdgeHandle, const unsigned int direction)> edge_color_;
};

class HalfedgeShader : public BasicShader {
public:
	HalfedgeShader(const std::string vert,
		const std::string frag, 
		const std::string geom)
		: BasicShader(vert, frag, geom),
		halfedge_color_([](const HalfedgeHandle e, const bool opp) { return Color::Empty(); }) {}
	HalfedgeShader(const std::string vert, 
		const std::string frag)
		: HalfedgeShader(vert, frag, "") {}

	void Update() override {
		if (updated_) return;
		updated_ = true;
		size_t i = 0; // line index;
		Eigen::Matrix4Xf line_colors(4, cmesh().n_halfedges() * 2);
		for (const HalfedgeHandle he : cmesh().halfedges()) {
			line_colors.col(i + 0) = halfedge_color_(he, false);
			line_colors.col(i + 1) = halfedge_color_(he, true);
			i += 2;
		}
		assert(i == cmesh().n_halfedges() * 2);
		Bind();
		UploadAttrib("vert_color", line_colors);
	}

	void SetColorFunc(const std::function<Color(const HalfedgeHandle e, const bool opp)> f) {
		halfedge_color_ = f;
		Invalidate();
	}

private:
	std::function<Color(const HalfedgeHandle he, const bool opp)> halfedge_color_;
};


class CustomShader : public BasicShader {
public:
	CustomShader(const std::string vert,
		const std::string frag, 
		const std::string geom)
		: BasicShader(vert, frag, geom),
		color_func_([]() { return Eigen::Matrix4Xf(); }) {}
	CustomShader(const std::string vert, 
		const std::string frag)
		: CustomShader(vert, frag, "") {}

	void Update() override {
		if (updated_) return;
		updated_ = true;
		Bind();
		UploadAttrib("vert_color", color_func_());
	}

	void SetColorFunc(const std::function<const Eigen::Matrix4Xf()> f) {
		color_func_ = f;
		Invalidate();
	}

private:
	std::function<const Eigen::Matrix4Xf()> color_func_;
};
