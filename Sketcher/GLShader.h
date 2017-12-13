#pragma once

#include <string>
#include <map>

#include <GL/gl3w.h>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "CommonDefs.h"

template <typename T> struct type_traits;
template <> struct type_traits<uint32_t> { enum { type = GL_UNSIGNED_INT, integral = 1 }; };
template <> struct type_traits<int32_t> { enum { type = GL_INT, integral = 1 }; };
template <> struct type_traits<uint16_t> { enum { type = GL_UNSIGNED_SHORT, integral = 1 }; };
template <> struct type_traits<int16_t> { enum { type = GL_SHORT, integral = 1 }; };
template <> struct type_traits<uint8_t> { enum { type = GL_UNSIGNED_BYTE, integral = 1 }; };
template <> struct type_traits<int8_t> { enum { type = GL_BYTE, integral = 1 }; };
template <> struct type_traits<double> { enum { type = GL_DOUBLE, integral = 0 }; };
template <> struct type_traits<float> { enum { type = GL_FLOAT, integral = 0 }; };


class GLShader {
private:
	static GLuint createShader_helper(GLint type,
		const std::string &defines, std::string shader_string);

public:
	/// Create an unitialized OpenGL shader
	GLShader()
		: initialized(false), 
		mPrimitiveCount(0), mVertexShader(0), mFragmentShader(0), 
		mGeometryShader(0), mProgramShader(0), mVertexArrayObject(0) {}

	/// Initialize the shader using the specified source strings
	bool Init(const std::string &vertex_str,
		const std::string &fragment_str,
		const std::string &geometry_str = "");

	/// Initialize the shader using the specified files on disk
	bool InitFromFiles(const std::string &vertex_fname,
		const std::string &fragment_fname,
		const std::string &geometry_fname = "");

	/// Set a preprocessor definition
	void Define(const std::string &key, const std::string &value) { mDefinitions[key] = value; }

	/// Select this shader for subsequent draw calls
	void Bind();

	/// Release underlying OpenGL objects
	void Free();

	/// Return the handle of a named shader attribute (-1 if it does not exist)
	GLint attrib(const std::string &name, bool warn = true) const;

	/// Return the handle of a uniform attribute (-1 if it does not exist)
	GLint uniform(const std::string &name, bool warn = true) const;

	/// Upload an Eigen matrix as a vertex buffer object (refreshing it as needed)
	template <typename Matrix> void UploadAttrib(const std::string &name, 
		const Matrix &M, int version = -1);

	/// Download a vertex buffer object into an Eigen matrix
	template <typename Matrix> void DownloadAttrib(const std::string &name, 
		Matrix &M);

	/// Upload an index buffer
	template <typename Matrix> void UploadIndices(const Matrix &M) {
		UploadAttrib("indices", M);
	}

	/// Set primitive count and type (e.g. GL_TRIANGLES, 3)
	void SetPrimitives(GLint type, GLuint count) {
		mPrimitiveType = type;
		mPrimitiveCount = count;
	}

	/// Invalidate the version numbers assiciated with attribute data
	void InvalidateAttribs();

	/// Completely free an existing attribute buffer
	void FreeAttrib(const std::string &name);

	/// Check if an attribute was registered a given name
	bool HasAttrib(const std::string &name) const {
		auto it = mBufferObjects.find(name);
		if (it == mBufferObjects.end())
			return false;
		return true;
	}

	/// Create a symbolic link to an attribute of another GLShader. This avoids duplicating unnecessary data
	void ShareAttrib(const GLShader &otherShader, const std::string &name, const std::string &as = "");

	/// Return the version number of a given attribute
	int AttribVersion(const std::string &name) const {
		auto it = mBufferObjects.find(name);
		if (it == mBufferObjects.end())
			return -1;
		return it->second.version;
	}

	/// Reset the version number of a given attribute
	void ResetAttribVersion(const std::string &name) {
		auto it = mBufferObjects.find(name);
		if (it != mBufferObjects.end())
			it->second.version = -1;
	}

	/// Draw a sequence of primitives
	void DrawArray(int type, uint32_t offset, uint32_t count) const;
	/// Draw using the previously defined primitives
	void DrawArray() const { DrawArray(mPrimitiveType, 0, mPrimitiveCount); }

	/// Draw a sequence of primitives using a previously uploaded index buffer
	void DrawIndexed(int type, uint32_t offset, uint32_t count) const;
	/// Draw using the previously defined primitives
	void DrawIndexed() const { DrawIndexed(mPrimitiveType, 0, mPrimitiveCount); }

	/// Initialize a uniform parameter with a 4x4 matrix
	void SetUniform(const std::string &name, const glm::mat4 &mat, bool warn = true) {
		glUniformMatrix4fv(uniform(name, warn), 1, GL_FALSE, glm::value_ptr(mat));
	}

	/// Initialize a uniform parameter with a 3x3 matrix
	void SetUniform(const std::string &name, const glm::mat3 &mat, bool warn = true) {
		glUniformMatrix3fv(uniform(name, warn), 1, GL_FALSE, glm::value_ptr(mat));
	}

	/// Initialize a uniform parameter with an integer value
	void SetUniform(const std::string &name, int value, bool warn = true) {
		glUniform1i(uniform(name, warn), value);
	}

	/// Initialize a uniform parameter with a float value
	void SetUniform(const std::string &name, float value, bool warn = true) {
		glUniform1f(uniform(name, warn), value);
	}

	/// Initialize a uniform parameter with a 2D vector
	void SetUniform(const std::string &name, const glm::vec2 &v, bool warn = true) {
		glUniform2f(uniform(name, warn), v.x, v.y);
	}

	/// Initialize a uniform parameter with a 3D vector
	void SetUniform(const std::string &name, const glm::vec3 &v, bool warn = true) {
		glUniform3f(uniform(name, warn), v.x, v.y, v.z);
	}

	/// Initialize a uniform parameter with a 4D vector
	void SetUniform(const std::string &name, const glm::vec4 &v, bool warn = true) {
		glUniform4f(uniform(name, warn), v.x, v.y, v.z, v.w);
	}

	/// Return the size of all registered buffers in bytes
	size_t BufferSize() const {
		size_t size = 0;
		for (auto const &buf : mBufferObjects)
			size += buf.second.size;
		return size;
	}

	/// Returns true if initialized
	bool Initialized() const { return initialized; }

protected:
	void UploadAttrib(const std::string &name, uint32_t size, int dim,
		uint32_t compSize, GLuint glType, bool integral,
		const uint8_t *data, int version = -1);
	void DownloadAttrib(const std::string &name, uint32_t size, int dim,
		uint32_t compSize, GLuint glType, uint8_t *data);
protected:
	struct Buffer {
		GLuint id;
		GLuint glType;
		GLuint dim;
		GLuint compSize;
		GLuint size;
		int version;
	};
	bool initialized;
	GLint mPrimitiveType;
	GLuint mPrimitiveCount;
	GLuint mVertexShader;
	GLuint mFragmentShader;
	GLuint mGeometryShader;
	GLuint mProgramShader;
	GLuint mVertexArrayObject;
	std::map<std::string, Buffer> mBufferObjects;
	std::map<std::string, std::string> mDefinitions;
};

template<typename Matrix>
void GLShader::UploadAttrib(const std::string & name, 
	const Matrix& M, int version) {
	uint32_t compSize = sizeof(typename Matrix::Scalar);
	GLuint glType = (GLuint)type_traits<typename Matrix::Scalar>::type;
	bool integral = (bool)type_traits<typename Matrix::Scalar>::integral;

	UploadAttrib(name, (uint32_t)M.size(), (int)M.rows(), compSize,
		glType, integral, (const uint8_t *)M.data(), version);
}

template<typename Matrix>
inline void GLShader::DownloadAttrib(const std::string & name, Matrix & M) {
	uint32_t compSize = sizeof(typename Matrix::Scalar);
	GLuint glType = (GLuint)type_traits<typename Matrix::Scalar>::type;

	auto it = mBufferObjects.find(name);
	if (it == mBufferObjects.end())
		throw std::runtime_error("downloadAttrib(" + M + ", " + name + ") : buffer not found!");

	const Buffer &buf = it->second;
	M.resize(buf.dim, buf.size / buf.dim);

	downloadAttrib(name, M.size(), M.rows(), compSize, glType, (uint8_t *)M.data());
}
