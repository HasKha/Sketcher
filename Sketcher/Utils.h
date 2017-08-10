#pragma once

/*
This file contains a bunch of vector operations that may be useful everywhere
*/

#include "CommonDefs.h"

class Utils {
public:
	// ==== Vector -> angle ====
	// returns the cosine of the angle between the two vectors
	// the result is in the range [-1, 1]
	static double CosAngleBetween(const Vec3d& v1, const Vec3d& v2);
	static double CosAngleBetween(const Vector3d& v1, const Vector3d& v2);

	// returns the angle, in radians, between the two vectors
	// the result is in the range [0, PI]
	static double AngleBetween(const Vec3d& v1, const Vec3d& v2);
	static double AngleBetween(const Vector3d& v1, const Vector3d& v2);

	static double ProjectedCosAngleBetween(const Vec3d& v1, const Vec3d& v2, const Vec3d& n);
	static double ProjectedAngleBetween(const Vec3d& v1, const Vec3d& v2, const Vec3d& n);

	// two implementations of the gaussian
	// first one is a simple gaussian with sigma
	static double Gauss(double x, double sigma);
	// this one has a center which is 'b' or 'u', and
	static double Gauss(double x, double center, double sigma);

	static inline double ToDeg() { return 180 / M_PI; }
	static inline double ToDeg(double rad) { return 180 * rad / M_PI; }
	static inline float ToDeg(float rad) { return 180.0f * rad / (float)M_PI; }

	static inline double ToRad() { return M_PI / 180; }
	static inline double ToRad(double rad) { return M_PI * rad / 180; }
	static inline float ToRad(float rad) { return (float)M_PI * rad / 180.0f; }

	// Rotates vector v around axis until it lays on the plane described by normal n
	// Assumes that n is normalized
	static Vector3d RotateOnPlane(const Vector3d& v, const Vector3d& n, const Vector3d& axis);

	static double GetXthPerc(std::vector<double>& v, double perc);
	static double Get90thPerc(std::vector<double>& v);

	static float percentile;
	static bool use_projection;
private:
	// Projects vector v to the plane described by the normal n
	// Assumes that n is normalized
	static Vector3d ProjectOnPlane(const Vector3d& v, const Vector3d& n);
	static Vec3d ProjectOnPlane(const Vec3d& v, const Vec3d& n);
};
