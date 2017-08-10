#include "Converters.h"

namespace Converters {
	Vector3d convert(Vec3d v) {
		return Vector3d(v[0], v[1], v[2]);
	}

	Vec3d convert(Vector3d v) {
		return Vec3d(v.x(), v.y(), v.z());
	}

	Vector3f d2f(Vec3d v) {
		return Vector3d(v[0], v[1], v[2]).cast<float>();
	}

	Vector4f normal2color(Vec3d n, double alpha) {
		return Eigen::Vector4d(
			std::abs(n[0]),
			std::abs(n[1]),
			std::abs(n[2]),
			alpha).cast<float>();
	}

	Vector4f normal2color(Vec3d n) {
		return normal2color(n, 1.0);
	}

	ImColor convert(Color c) {
		return ImColor(c.r(), c.g(), c.b(), c.a());
	}
	Color convert(ImColor c) {
		return Color(c.Value.x, c.Value.y, c.Value.z, c.Value.w);
	}
};

