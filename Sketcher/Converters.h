#pragma once

#include "CommonDefs.h"

#include "Color.h"
#include <imgui.h>

namespace Converters {
	Vector3d convert(Vec3d v);
	Vec3d convert(Vector3d v);
	
	Vector3f d2f(Vec3d v);

	Vector4f normal2color(Vec3d n, double alpha);
	Vector4f normal2color(Vec3d n);

	ImColor convert(Color c);
	Color convert(ImColor c);
};
