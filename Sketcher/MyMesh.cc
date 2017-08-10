#include "MyMesh.h"

#include <iostream>
#include <vector>

MyMesh mesh_;
MyMesh backup_;
const MyMesh& cmesh() { return mesh_; }
const MyMesh& cbackup() { return backup_; }
MyMesh& mesh() { return mesh_; }
void DoBackup() { backup_ = mesh_; }
void RestoreBackup() { mesh_ = backup_; }

void MyMesh::initialize() {
	update_face_normals();
	update_vertex_normals();
	calc_average_edge_length();
}

double MyMesh::calc_average_edge_length() {
	average_edge_length_ = 0;
	for (const EdgeHandle e : cmesh().edges()) {
		average_edge_length_ += calc_edge_length(cmesh().halfedge_handle(e, 0));
	}
	average_edge_length_ /= n_edges();
	return average_edge_length_;
}

bool MyMesh::is_on_feature(const VertexHandle v) const {
	for (EdgeHandle e : ve_range(v)) {
		if (is_feature(e)) return true;
	}
	return false;
}

bool MyMesh::is_across_feature(const HalfedgeHandle in, const HalfedgeHandle out) const {
	assert(to_vertex_handle(in) == from_vertex_handle(out));
	const VertexHandle v = to_vertex_handle(in);
	const EdgeHandle ein = edge_handle(in);
	const EdgeHandle eout = edge_handle(out);
	for (const EdgeHandle e : ve_range(v)) {
		if (e != ein && e != eout && is_feature(e)) return true;
	}
	return false;
}

double MyMesh::surface_angle_left(const HalfedgeHandle in, const HalfedgeHandle out) const {
    assert(in.is_valid());
    assert(out.is_valid());
    assert(to_vertex_handle(in) == from_vertex_handle(out));
    
    double angle = 0;
    HalfedgeHandle current = in;
    while (current != opposite_halfedge_handle(out)) {
		if (is_boundary(current)) return HUGE_VAL;
		if (!face_handle(current).is_valid()) return HUGE_VAL;
		const HalfedgeHandle next = next_halfedge_handle(current);
		if (!next.is_valid()) return HUGE_VAL;
		const HalfedgeHandle opp = opposite_halfedge_handle(next);
		if (!opp.is_valid()) return HUGE_VAL;
        angle += angle_between(current, opp);
        current = opp;
    }
    return angle;
}

double MyMesh::surface_angle_right(const HalfedgeHandle in, const HalfedgeHandle out) const {
    assert(in.is_valid());
    assert(out.is_valid());
    assert(to_vertex_handle(in) == from_vertex_handle(out));
    
    double angle = 0;
    HalfedgeHandle current = in;
    
    while (current != opposite_halfedge_handle(out)) {
		const HalfedgeHandle opp = opposite_halfedge_handle(current);
		if (is_boundary(opp)) return HUGE_VAL;
		if (!face_handle(opp).is_valid()) return HUGE_VAL;
		if (!opp.is_valid()) return HUGE_VAL;
		const HalfedgeHandle prev = prev_halfedge_handle(opp);
		if (!prev.is_valid()) return HUGE_VAL;
        angle += angle_between(current, prev);
        current = prev;
    }
    return angle;
}


double MyMesh::surface_angle(const HalfedgeHandle in, const HalfedgeHandle out) const {
	double angle_left = surface_angle_left(in, out);
	double angle_right = surface_angle_right(in, out);
	assert(!isnan(angle_left));
	assert(!isnan(angle_right));
	const double out_of_range = 100; // number that means the angle value is no longer an angle (in radians)
	if (angle_left > out_of_range && angle_right > out_of_range) {
		return HUGE_VAL;
	} else if (angle_left > out_of_range) {
		return M_PI - angle_right;
	} else if (angle_right > out_of_range) {
		return angle_left - M_PI;
	} else {
		angle_left = angle_left - M_PI;
		angle_right = M_PI - angle_right;
		return (angle_left + angle_right) / 2;
	}
}

double MyMesh::surface_angle_score(const HalfedgeHandle in, const HalfedgeHandle out) const {
	return std::abs(surface_angle(in, out));
}

HalfedgeHandle MyMesh::halfedge_opposite_to(const FaceHandle f1, const FaceHandle f2) const {
	for (ConstFaceHalfedgeIter it = cfh_begin(f1); it.is_valid(); ++it) {
		const HalfedgeHandle he = *it;
		const HalfedgeHandle opp = opposite_halfedge_handle(he);
		if (!opp.is_valid()) continue;
		if (face_handle(opp) == f2) {
			return he;
		}
	}
	return HalfedgeHandle();
}

EdgeHandle MyMesh::edge_between(const FaceHandle f1, const FaceHandle f2) const {
	const HalfedgeHandle he = halfedge_opposite_to(f1, f2);
	if (!he.is_valid()) return EdgeHandle();
	return edge_handle(he);
}

HalfedgeHandle MyMesh::halfedge_between(const VertexHandle from, const VertexHandle to) const {
	for (ConstVertexOHalfedgeIter it = cvoh_begin(from); it.is_valid(); ++it) {
		if (to_vertex_handle(*it) == to) return *it;
	}
	return HalfedgeHandle();
}


OpenMesh::Vec3d MyMesh::compute_center() const {
	OpenMesh::Vec3d bbMin, bbMax;
	bbMin = bbMax = point(VertexHandle(0));
	for (VertexHandle v : vertices()) {
		bbMin.minimize(point(v));
		bbMax.maximize(point(v));
	}
	return (bbMin + bbMax) / 2;
}

double MyMesh::compute_size() const {
	OpenMesh::Vec3d bbMin, bbMax;
	bbMin = bbMax = point(VertexHandle(0));
	for (VertexHandle v : vertices()) {
		bbMin.minimize(point(v));
		bbMax.maximize(point(v));
	}
	OpenMesh::Vec3d diag = (bbMin - bbMax);
	return diag.norm();
}

void MyMesh::move_to_origin() {
	const OpenMesh::Vec3d center = compute_center();
	for (VertexHandle v : vertices()) {
		point(v) -= center;
	}
}

void MyMesh::normalize() {
	const double scale = compute_size();
	for (VertexHandle v : vertices()) {
		set_point(v, point(v) / scale);
	}
}

OpenMesh::Vec3d MyMesh::normal(const FaceHandle f) const {
	return OpenMesh::PolyMesh_ArrayKernelT<MyTraits>::normal(f);
}

OpenMesh::Vec3d MyMesh::normal(const VertexHandle v) const {
	return OpenMesh::PolyMesh_ArrayKernelT<MyTraits>::normal(v);
}

OpenMesh::Vec3d MyMesh::normal(const HalfedgeHandle he) const {
	const FaceHandle f = face_handle(he);
	if (f.is_valid()) {
		return normal(f);
	} else {
		return OpenMesh::Vec3d(0, 0, 0);
	}
}

OpenMesh::Vec3d MyMesh::normal(const EdgeHandle e) const {
	const HalfedgeHandle he0 = halfedge_handle(e, 0);
	const HalfedgeHandle he1 = halfedge_handle(e, 1);
	assert(!is_boundary(he0) || !is_boundary(he1)); // free edge, bad
	if (is_boundary(he0)) {
		return normal(face_handle(he1));
	} else if (is_boundary(he1)) {
		return normal(face_handle(he0));
	} else {
		return (normal(face_handle(he0)) + normal(face_handle(he1))).normalized();
	}
}

OpenMesh::Vec3d MyMesh::safe_normal(const HalfedgeHandle he) const {
	if (is_boundary(he)) {
		return normal(opposite_halfedge_handle(he));
	} else {
		return normal(he);
	}
}

OpenMesh::Vec3d MyMesh::good_normal(const VertexHandle v, const FaceHandle f) const {
	if (is_on_feature(v)) {
		OpenMesh::Vec3d n = normal(f);
		for (ConstVertexFaceIter it = cvf_begin(v); it.is_valid(); ++it) {
			if (*it == f) continue;
			const EdgeHandle between = edge_between(*it, f);
			if (between.is_valid() && !is_feature(between)) {
				n += normal(*it);
			}
		}
		return n.normalized();
	} else {
		OpenMesh::Vec3d n = normal(v);
		//for (VertexHandle neighbor : vv_range(v)) {
		//	if (!is_on_feature(neighbor)) {
		//		n += normal(neighbor);
		//	}
		//}
		return n.normalized();
	}
}

Vec3d MyMesh::midpoint(const HalfedgeHandle he) const {
	const VertexHandle from = from_vertex_handle(he);
	const VertexHandle to = to_vertex_handle(he);
	return (point(to) + point(from)) / 2;
}

Vec3d MyMesh::midpoint(const EdgeHandle e) const {
	return midpoint(halfedge_handle(e, 0));
}

Vec3d MyMesh::midpoint(const FaceHandle f) const {
	Vec3d ret(0, 0, 0);
	int count = 0;
	for (MyMesh::ConstFaceVertexIter it = cfv_begin(f); it.is_valid(); ++it) {
		const VertexHandle v = *it;
		ret += point(v);
		++count;
	}
	return ret / count;
}

Vec3d MyMesh::vector(const HalfedgeHandle he) const {
	const VertexHandle from = from_vertex_handle(he);
	const VertexHandle to = to_vertex_handle(he);
	return point(to) - point(from);
}
