#include "Utils.h"

bool Utils::use_projection = true;
float Utils::percentile = 0.90f;

double Utils::CosAngleBetween(const Vector3d& v1, const Vector3d& v2) {
	double dot = v1.normalized().dot(v2.normalized());
	if (dot < -1) return -1;
	if (dot > 1) return 1;
	return dot;
}
double Utils::CosAngleBetween(const Vec3d& v1, const Vec3d& v2) {
	double dot = OpenMesh::dot(v1.normalized(), v2.normalized());
	if (dot < -1) return -1;
	if (dot > 1) return 1;
	return dot;
}

double Utils::AngleBetween(const Vector3d& v1, const Vector3d& v2) {
	return acos(CosAngleBetween(v1, v2));
}
double Utils::AngleBetween(const Vec3d& v1, const Vec3d& v2) {
	return acos(CosAngleBetween(v1, v2));
}

double Utils::ProjectedCosAngleBetween(const Vec3d& v1, const Vec3d& v2, const Vec3d& n) {
	if (use_projection) {
		const Vec3d proj1 = Utils::ProjectOnPlane(v1, n);
		const Vec3d proj2 = Utils::ProjectOnPlane(v2, n);
		return CosAngleBetween(proj1, proj2);
	} else {
		return CosAngleBetween(v1, v2);
	}
}
double Utils::ProjectedAngleBetween(const Vec3d& v1, const Vec3d& v2, const Vec3d& n) {
	return acos(ProjectedCosAngleBetween(v1, v2, n));
}

double Utils::Gauss(double x, double sigma) {
	double exponent = (x * x) / (2 * sigma * sigma);
	return exp(-exponent);
}

double Utils::Gauss(double x, double center, double sigma) {
	double x2 = (x - center) * (x - center);
	double exponent = x2 / (2 * sigma * sigma);
	return exp(-exponent);
}

Vector3d Utils::ProjectOnPlane(const Vector3d& v, const Vector3d& n) {
	return v - v.dot(n) * n;
}
Vec3d Utils::ProjectOnPlane(const Vec3d& v, const Vec3d& n) {
	return v - (v | n) * n;
}

Vector3d Utils::RotateOnPlane(const Vector3d& v,
	const Vector3d& n, const Vector3d& axis) {
	assert(false); // TODO
	return Vector3d(1.0, 0.0, 0.0);
}

double Utils::Get90thPerc(std::vector<double>& vector) {
	return GetXthPerc(vector, percentile);
}
double Utils::GetXthPerc(std::vector<double>& vector, double perc) {
	assert(perc >= 0 && perc <= 1);
	if (vector.size() == 0) return 0;
	std::sort(vector.begin(), vector.end());
	long idx = std::lround((vector.size() - 1) * perc);
	assert(idx >= 0 && idx < vector.size());
	return vector[idx];
}
