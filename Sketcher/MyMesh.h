#pragma once

#include <vector>

#include <OpenMesh/Core/IO/MeshIO.hh>
#include <OpenMesh/Core/Mesh/Handles.hh>

#include "CommonDefs.h"

#include "Color.h"
#include "Utils.h"

// if the cosine of the angle between normals of 
// two adjacent faces is greater than this, then 
// the edge between them is a hard edge
#define HARD_EDGE_MAX_NORMAL_COSINE 0.7

struct MyTraits : public OpenMesh::DefaultTraits {
	using Point = OpenMesh::Vec3d;
	using Normal = OpenMesh::Vec3d;

	VertexAttributes(OpenMesh::Attributes::Normal 
		| OpenMesh::Attributes::Status);
    VertexTraits {};

	FaceAttributes(OpenMesh::Attributes::Normal
		| OpenMesh::Attributes::Status);
    FaceTraits {};

	HalfedgeAttributes( OpenMesh::Attributes::Normal
		| OpenMesh::Attributes::Status);
    HalfedgeTraits {};

	EdgeAttributes(OpenMesh::Attributes::Status);
    EdgeTraits {};
};

class MyMesh : public OpenMesh::PolyMesh_ArrayKernelT<MyTraits> {
public:
	MyMesh() {}

	float feature_max_angle = 45.0f;
	bool render_ready = false;

    void initialize();
    
	// ==== General Queries ====
	bool is_feature(const EdgeHandle e) const {
		return is_feature(halfedge_handle(e, 0)); }
	bool is_feature(const HalfedgeHandle he) const { 
		return is_estimated_feature_edge(he, Utils::ToRad(feature_max_angle)); }
	bool is_on_feature(const VertexHandle v) const;
	bool is_across_feature(const HalfedgeHandle in, const HalfedgeHandle out) const;

    inline bool is_quad(const FaceHandle f) const { return valence(f) == 4; }
    inline bool is_triangle(const FaceHandle f) const { return valence(f) == 3; }
    inline bool is_singularity(const VertexHandle v) const { return valence(v) != 4; }
  
    // returns the vector along the halfedge, in the same direction as the halfedge
	OpenMesh::Vec3d vector(const HalfedgeHandle he) const;
	OpenMesh::Vec3d vector(const EdgeHandle e, int i) const { return vector(halfedge_handle(e, i)); }

	// Returns the midpoint of an edge (e.g. (to - from) / 2)
	Vec3d midpoint(const HalfedgeHandle he) const;
	Vec3d midpoint(const EdgeHandle e) const;
	Vec3d midpoint(const FaceHandle f) const;

	// halfedge, face, and vertex normals
	OpenMesh::Vec3d normal(const HalfedgeHandle he) const;
	OpenMesh::Vec3d normal(const EdgeHandle e) const;
	OpenMesh::Vec3d normal(const FaceHandle f) const;
	OpenMesh::Vec3d normal(const VertexHandle v) const;

	// returns a good normal for rendering vertex v for face f
	OpenMesh::Vec3d good_normal(const VertexHandle v, const FaceHandle f) const;
	// halfedge normal, fallbacks to opposite halfedge if it's boundary
	OpenMesh::Vec3d safe_normal(const HalfedgeHandle he) const;
    
	// ==== Angles ====
	// returns the angle, in radians, between the vectors along he1 and he2
	inline double angle_between(const HalfedgeHandle he1, const HalfedgeHandle he2) const {
		return Utils::AngleBetween(vector(he1), vector(he2));
	}
	inline double cos_angle_between(const HalfedgeHandle he1, const HalfedgeHandle he2) const {
		return Utils::CosAngleBetween(vector(he1), vector(he2));
	}

	// computes the surface angles in radians
    double surface_angle_left(const HalfedgeHandle in, const HalfedgeHandle out) const;
    double surface_angle_right(const HalfedgeHandle in, const HalfedgeHandle out) const;
	// returns the surface angle as a difference from straight
	// 0 for 180deg angles, positive for curving right, negative for curving left
	// result is in range [-PI, PI]
	double surface_angle(const HalfedgeHandle in, const HalfedgeHandle out) const;
	// returns a score where 0 is perfectly straight, higher is more curved
	double surface_angle_score(const HalfedgeHandle in, const HalfedgeHandle out) const;
    
    // computes the average edge length across the whole mesh
	double calc_average_edge_length();
	double average_edge_length() const { return average_edge_length_; }
    
	// returns the edge between the two given faces
	// returns invalid edge if faces are not neighboring
	EdgeHandle edge_between(const FaceHandle f1, const FaceHandle f2) const;
	HalfedgeHandle halfedge_between(const VertexHandle from, const VertexHandle to) const;

	// returns the halfedge, belonging to f1, opposite to f2
	// returns invalid halfedge if faces are not neighboring
	HalfedgeHandle halfedge_opposite_to(const FaceHandle f1, const FaceHandle f2) const;

	// Computes the center of the mesh
	// (just center of bounding box)
	OpenMesh::Vec3d compute_center() const;
	// compute the length of the bounding box diagonal
	double compute_size() const;
	// scales the mesh such that the size is 1
	void normalize();

	// Moves the mesh to the origin
	// such that at the end center() = (0, 0, 0)
	void move_to_origin();

	double average_edge_length_ = 0;
};


// yes those are globals
// the other option would be to just make a singleton
// and then write MyMesh::instance() every time which is annoying
// so this is the same result but only requires mesh()
const MyMesh& cmesh();
const MyMesh& cbackup();
MyMesh& mesh();
void DoBackup();
void RestoreBackup();

// ---- data structures ----
using Point = MyMesh::Point;
// ---- Handles ----
using VertexHandle = MyMesh::VertexHandle;
using HalfedgeHandle = MyMesh::HalfedgeHandle;
using EdgeHandle = MyMesh::EdgeHandle;
using FaceHandle = MyMesh::FaceHandle;
// ---- Iterators ----
using EdgeIter = MyMesh::EdgeIter;
using VertexIter = MyMesh::VertexIter;
using FaceIter = MyMesh::FaceIter;
using VertexOHalfedgeIter = MyMesh::VertexOHalfedgeIter;
using VertexVertexIter = MyMesh::VertexVertexIter;


// --- Hashers and comparators ---
template<> struct std::equal_to<FaceHandle> {
    bool operator() (const FaceHandle& f1, const FaceHandle& f2) const noexcept {
        return f1.idx() == f2.idx();
    }
};
