#pragma once

#include <Eigen/Core>

#include <assert.h>
#include <cmath>
#include <stdlib.h>

//-----------------------------------------------------------------------------
/// Template class for colors with RGB-components.
/**
 * This is the main class for colors.
 * The type of the three components is variable.
 */
class Color : public Eigen::Vector4f {

    //-----------------------------------------------------------------------------
public:
	static Color Empty() { return Color(0, 0, 0, 0); }
	static Color Zero() { return Color(0, 0, 0, 0); }
	static Color Black() { return Color(0, 0, 0); }
	static Color White() { return Color(1, 1, 1); }

	static Color Red() { return Color(1, 0, 0); }
	static Color Green() { return Color(0, 1, 0); }
	static Color Blue() { return Color(0, 0, 1); }

	static Color Cyan() { return Color(0, 1, 1); }
	static Color Yellow() { return Color(1, 1, 0); }
	static Color Purple() { return Color(1, 0, 1); }

	Color(const Eigen::Vector4f& color) 
		: Eigen::Vector4f(color) {}

	Color(const float r, const float g, const float b, const float a) 
		: Color(Eigen::Vector4f(r, g, b, a)) {}

	Color(const float r, const float g, const float b) 
		: Color(r, g, b, 1.0f) {}

	Color(const float v) 
		: Color(v, v, v, 1.0f) {}

	Color() 
		: Color(0, 0, 0, 0) {}
	
	template <typename Derived> Color(const Eigen::MatrixBase<Derived>& p)
		: Eigen::Vector4f(p) {}

	template <typename Derived> Color &operator=(const Eigen::MatrixBase<Derived>& p) {
		this->Eigen::Vector4f::operator=(p);
		return *this;
	}

	float& r() { return x(); }
    float r() const { return x(); }

    float& g() { return y(); }
    float g() const { return y(); }

    float& b() { return z(); }
    float b() const { return z(); }

	float& a() { return coeffRef(3); }
	float a() const { return coeff(3); }

	Color operator+(const float v) const {
		return Color(r() + v, g() + v, b() + v, a());
	}
	Color operator-(const float v) const {
		return Color(r() - v, g() - v, b() - v, a());
	}
	Color operator/(const float v) const {
		return Color(r() / v, g() / v, b() / v, a());
	}
	Color operator*(const float v) const {
		return Color(r() * v, g() * v, b() * v, a());
	}

	Color operator+(const Color c) const {
		return Color(r() + c.r(), g() + c.g(), b() + c.b(), std::max(a(), c.a()));
	}
	Color operator-(const Color c) const {
		return Color(r() - c.r(), g() - c.g(), b() - c.b(), std::max(a(), c.a()));
	}

    /**
     * Returns the clamped color.
     * @return the \b clamped color
     */
    void Clamp() {
        if (r() < 0.0) r() = 0.0;
        if (r() > 1.0) r() = 1.0;
        if (g() < 0.0) g() = 0.0;
        if (g() > 1.0) g() = 1.0;
        if (b() < 0.0) b() = 0.0;
        if (b() > 1.0) b() = 1.0;
    }

    inline bool IsWhite() { return (r() + g() + b()) >=3; }
    
    inline bool IsBlack() { return (r() == 0 && g() == 0 && b() == 0); }

	float distance(const Color c) const {
		return std::abs(c.r() - r())
			+ std::abs(c.g() - g())
			+ std::abs(c.b() - b());
	}

    /**
     * Set a random colour.
     * Warning: no srand() init is done!
     */
	static void ResetSeed(int seed = 0) {
		srand(seed);
	}
    static const Color Random() {
		//static float tau = 0.5f;
		//tau += (1.25f + (float)rand() / RAND_MAX);
		float tau = (float)(2 * M_PI * rand() / RAND_MAX);

		const float value = static_cast<float>(M_PI) / 3.0f;
		
        float center = 0.3f; // magic numbers!
        float width = 0.3f;
		return Color(	std::sin(tau + 0.0f * value) * width + center,
						std::sin(tau + 2.0f * value) * width + center,
						std::sin(tau + 4.0f * value) * width + center);
    }

};
