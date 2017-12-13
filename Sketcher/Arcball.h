/*
Taken from NanoGUI
*/

#include <Eigen/Geometry>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/type_ptr.hpp>

using Eigen::Vector2i;
using Eigen::Quaternionf;
using Eigen::Vector3f;
using Eigen::Matrix4f;

/// Arcball helper class to interactively rotate objects on-screen
struct Arcball {
	Arcball(float speedFactor = 2.0f)
		: mActive(false), mLastPos(Vector2i::Zero()), mSize(Vector2i::Zero()),
		mQuat(Quaternionf::Identity()),
		mIncr(Quaternionf::Identity()),
		mSpeedFactor(speedFactor) {}

	Arcball(const Quaternionf &quat)
		: mActive(false), mLastPos(Vector2i::Zero()), mSize(Vector2i::Zero()),
		mQuat(quat),
		mIncr(Quaternionf::Identity()),
		mSpeedFactor(2.0f) {}

	Quaternionf &state() { return mQuat; }

	void setState(const Quaternionf &state) {
		mActive = false;
		mLastPos = Vector2i::Zero();
		mQuat = state;
		mIncr = Quaternionf::Identity();
	}

	void setSize(Vector2i size) { mSize = size; }
	const Vector2i &size() const { return mSize; }
	void setSpeedFactor(float speedFactor) { mSpeedFactor = speedFactor; }
	float speedFactor() const { return mSpeedFactor; }
	bool active() const { return mActive; }

	void button(Vector2i pos, bool pressed) {
		mActive = pressed;
		mLastPos = pos;
		if (!mActive)
			mQuat = (mIncr * mQuat).normalized();
		mIncr = Quaternionf::Identity();
	}

	bool motion(Vector2i pos) {
		if (!mActive)
			return false;

		/* Based on the rotation controller form AntTweakBar */
		float invMinDim = 1.0f / mSize.minCoeff();
		float w = (float)mSize.x(), h = (float)mSize.y();

		float ox = (mSpeedFactor * (2 * mLastPos.x() - w) + w) - w - 1.0f;
		float tx = (mSpeedFactor * (2 * pos.x() - w) + w) - w - 1.0f;
		float oy = (mSpeedFactor * (h - 2 * mLastPos.y()) + h) - h - 1.0f;
		float ty = (mSpeedFactor * (h - 2 * pos.y()) + h) - h - 1.0f;

		ox *= invMinDim; oy *= invMinDim;
		tx *= invMinDim; ty *= invMinDim;

		Vector3f v0(ox, oy, 1.0f), v1(tx, ty, 1.0f);
		if (v0.squaredNorm() > 1e-4f && v1.squaredNorm() > 1e-4f) {
			v0.normalize(); v1.normalize();
			Vector3f axis = v0.cross(v1);
			float sa = std::sqrt(axis.dot(axis)),
				ca = v0.dot(v1),
				angle = std::atan2(sa, ca);
			if (tx*tx + ty*ty > 1.0f)
				angle *= 1.0f + 0.2f * (std::sqrt(tx*tx + ty*ty) - 1.0f);
			mIncr = Eigen::AngleAxisf(angle, axis.normalized());
			if (!std::isfinite(mIncr.norm()))
				mIncr = Quaternionf::Identity();
		}
		return true;
	}

	Matrix4f matrix() const {
		Matrix4f result2 = Matrix4f::Identity();
		result2.block<3, 3>(0, 0) = (mIncr * mQuat).toRotationMatrix();
		return result2;
	}

protected:
	bool mActive;
	Vector2i mLastPos;
	Vector2i mSize;
	Quaternionf mQuat, mIncr;
	float mSpeedFactor;
};