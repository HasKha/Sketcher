#pragma once

#include <Eigen/Core>
#include <Eigen/Geometry>
#include <OpenMesh/Core/Mesh/Traits.hh>
#include <OpenMesh/Core/Mesh/PolyMesh_ArrayKernelT.hh>

#define EPSILON 1e-6

enum Render {
	Shaded,
	Solid,

	DebugFace,
	DebugEdge,

	Wireframe,
	SingleEdge,

	FeaturesEdges,	// aka hardedges
	BoundaryEdges,	// aka boundaries

	PickerVertex,
	PickerEdge,
	PickerFace,

	VertexNormals,
	EdgeNormals,
	FaceNormals,

	N_RENDER_MODES
};

using OpenMesh::Vec3d;
using OpenMesh::Vec4d;

using Eigen::Vector3f;
using Eigen::Vector4f;

using Eigen::Vector3d;
using Eigen::Vector4d;

using Eigen::Matrix3f;
using Eigen::Matrix4f;
using Eigen::MatrixXf;

using Eigen::Quaterniond;

