#include "GLShader.h"

#include <iostream>
#include <iterator>
#include <fstream>

#include "Log.h"

GLuint GLShader::createShader_helper(GLint type,
	const std::string &defines,
	std::string shader_string) {
	if (shader_string.empty())
		return (GLuint)0;

	if (!defines.empty()) {
		if (shader_string.length() > 8 && shader_string.substr(0, 8) == "#version") {
			std::istringstream iss(shader_string);
			std::ostringstream oss;
			std::string line;
			std::getline(iss, line);
			oss << line << std::endl;
			oss << defines;
			while (std::getline(iss, line))
				oss << line << std::endl;
			shader_string = oss.str();
		} else {
			shader_string = defines + shader_string;
		}
	}

	GLuint id = glCreateShader(type);
	const char *shader_string_const = shader_string.c_str();
	glShaderSource(id, 1, &shader_string_const, nullptr);
	glCompileShader(id);

	GLint status;
	glGetShaderiv(id, GL_COMPILE_STATUS, &status);

	if (status != GL_TRUE) {
		char buffer[512];
		std::cerr << "Error while compiling ";
		if (type == GL_VERTEX_SHADER)
			std::cerr << "vertex shader";
		else if (type == GL_FRAGMENT_SHADER)
			std::cerr << "fragment shader";
		else if (type == GL_GEOMETRY_SHADER)
			std::cerr << "geometry shader";
		std::cerr << shader_string << std::endl << std::endl;
		glGetShaderInfoLog(id, 512, nullptr, buffer);
		std::cerr << "Error: " << std::endl << buffer << std::endl;
		throw std::runtime_error("Shader compilation failed!");
	}

	return id;
}

bool GLShader::InitFromFiles(
	const std::string &vertex_fname,
	const std::string &fragment_fname,
	const std::string &geometry_fname) {
	auto file_to_string = [](const std::string &filename) -> std::string {
		if (filename.empty())
			return "";
		std::ifstream t(filename);
		return std::string((std::istreambuf_iterator<char>(t)),
			std::istreambuf_iterator<char>());
	};

	return Init(
		file_to_string(vertex_fname),
		file_to_string(fragment_fname),
		file_to_string(geometry_fname));
}

bool GLShader::Init(
	const std::string &vertex_str,
	const std::string &fragment_str,
	const std::string &geometry_str) {

	assert(!initialized);

	std::string defines;
	for (auto def : mDefinitions)
		defines += std::string("#define ") + def.first + std::string(" ") + def.second + "\n";

	glGenVertexArrays(1, &mVertexArrayObject);
	mVertexShader =
		createShader_helper(GL_VERTEX_SHADER, defines, vertex_str);
	mGeometryShader =
		createShader_helper(GL_GEOMETRY_SHADER, defines, geometry_str);
	mFragmentShader =
		createShader_helper(GL_FRAGMENT_SHADER, defines, fragment_str);

	if (!mVertexShader) {
		printf("Error: cannot create vertex shader '%s'\n", vertex_str.c_str());
		assert(false);
	}
	if (!mFragmentShader) {
		printf("Error: cannot create fragment shader '%s'\n", fragment_str.c_str());
		assert(false);
	}
	if (!geometry_str.empty() && !mGeometryShader) {
		printf("Error: cannot create geometry shader '%s'\n", fragment_str.c_str());
		assert(false);
	}

	mProgramShader = glCreateProgram();
	glAttachShader(mProgramShader, mVertexShader);
	glAttachShader(mProgramShader, mFragmentShader);

	if (mGeometryShader)
		glAttachShader(mProgramShader, mGeometryShader);

	glLinkProgram(mProgramShader);

	GLint status;
	glGetProgramiv(mProgramShader, GL_LINK_STATUS, &status);

	if (status != GL_TRUE) {
		char buffer[512];
		glGetProgramInfoLog(mProgramShader, 512, nullptr, buffer);
		std::cerr << "Linker error: " << std::endl << buffer << std::endl;
		mProgramShader = 0;
		throw std::runtime_error("Shader linking failed!");
	}

	glDetachShader(mProgramShader, mVertexShader);
	glDetachShader(mProgramShader, mFragmentShader);
	if (mGeometryShader)
		glDetachShader(mProgramShader, mGeometryShader);

	initialized = true;
	return true;
}

void GLShader::Bind() {
	glUseProgram(mProgramShader);
	glBindVertexArray(mVertexArrayObject);
}

GLint GLShader::attrib(const std::string &name, bool warn) const {
	GLint id = glGetAttribLocation(mProgramShader, name.c_str());
	return id;
}

GLint GLShader::uniform(const std::string &name, bool warn) const {
	GLint id = glGetUniformLocation(mProgramShader, name.c_str());
	return id;
}

void GLShader::UploadAttrib(const std::string &name, uint32_t size, int dim,
	uint32_t compSize, GLuint glType, bool integral, const uint8_t *data, int version) {
	int attribID = 0;
	if (name != "indices") {
		attribID = attrib(name);
		assert(attribID >= 0);
	}

	GLuint bufferID;
	auto it = mBufferObjects.find(name);
	if (it != mBufferObjects.end()) {
		Buffer &buffer = it->second;
		bufferID = it->second.id;
		buffer.version = version;
		buffer.size = size;
		buffer.compSize = compSize;
	} else {
		glGenBuffers(1, &bufferID);
		Buffer buffer;
		buffer.id = bufferID;
		buffer.glType = glType;
		buffer.dim = dim;
		buffer.compSize = compSize;
		buffer.size = size;
		buffer.version = version;
		mBufferObjects[name] = buffer;
	}
	size_t totalSize = (size_t)size * (size_t)compSize;

	if (name == "indices") {
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, bufferID);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, totalSize, data, GL_DYNAMIC_DRAW);
	} else {
		glBindBuffer(GL_ARRAY_BUFFER, bufferID);
		glBufferData(GL_ARRAY_BUFFER, totalSize, data, GL_DYNAMIC_DRAW);
		if (size == 0) {
			glDisableVertexAttribArray(attribID);
		} else {
			glEnableVertexAttribArray(attribID);
			glVertexAttribPointer(attribID, dim, glType, integral, 0, 0);
		}
	}
}

void GLShader::DownloadAttrib(const std::string &name, uint32_t size, int /* dim */,
	uint32_t compSize, GLuint /* glType */, uint8_t *data) {
	auto it = mBufferObjects.find(name);
	if (it == mBufferObjects.end())
		throw std::runtime_error("downloadAttrib(" + name + ") : buffer not found!");

	const Buffer &buf = it->second;
	if (buf.size != size || buf.compSize != compSize)
		throw std::runtime_error("downloadAttrib: size mismatch!");

	size_t totalSize = (size_t)size * (size_t)compSize;

	if (name == "indices") {
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buf.id);
		glGetBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, totalSize, data);
	} else {
		glBindBuffer(GL_ARRAY_BUFFER, buf.id);
		glGetBufferSubData(GL_ARRAY_BUFFER, 0, totalSize, data);
	}
}

void GLShader::ShareAttrib(const GLShader &otherShader, const std::string &name, const std::string &_as) {
	std::string as = _as.length() == 0 ? name : _as;
	auto it = otherShader.mBufferObjects.find(name);
	if (it == otherShader.mBufferObjects.end())
		throw std::runtime_error("shareAttribute(" + name + "): attribute not found!");
	const Buffer &buffer = it->second;

	if (name != "indices") {
		int attribID = attrib(as);
		if (attribID < 0)
			return;
		glEnableVertexAttribArray(attribID);
		glBindBuffer(GL_ARRAY_BUFFER, buffer.id);
		glVertexAttribPointer(attribID, buffer.dim, buffer.glType, buffer.compSize == 1 ? GL_TRUE : GL_FALSE, 0, 0);
	} else {
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buffer.id);
	}
}

void GLShader::InvalidateAttribs() {
	for (auto &buffer : mBufferObjects)
		buffer.second.version = -1;
}

void GLShader::FreeAttrib(const std::string &name) {
	auto it = mBufferObjects.find(name);
	if (it != mBufferObjects.end()) {
		glDeleteBuffers(1, &it->second.id);
		mBufferObjects.erase(it);
	}
}

void GLShader::DrawIndexed(int type, uint32_t offset_, uint32_t count_) const {
	if (count_ == 0) return;
	size_t offset = offset_;
	size_t count = count_;

	switch (type) {
	case GL_TRIANGLES: offset *= 3; count *= 3; break;
	case GL_LINES: offset *= 2; count *= 2; break;
	default: print("Shader DrawIndexed(), bad type!\n"); return;
	}

	glDrawElements(type, (GLsizei)count, GL_UNSIGNED_INT,
		(const void *)(offset * sizeof(uint32_t)));
}

void GLShader::DrawArray(int type, uint32_t offset, uint32_t count) const {
	if (count == 0) return;
	glDrawArrays(type, offset, count);
}

void GLShader::Free() {
	for (auto &buf : mBufferObjects)
		glDeleteBuffers(1, &buf.second.id);

	if (mVertexArrayObject)
		glDeleteVertexArrays(1, &mVertexArrayObject);

	glDeleteProgram(mProgramShader); mProgramShader = 0;
	glDeleteShader(mVertexShader);   mVertexShader = 0;
	glDeleteShader(mFragmentShader); mFragmentShader = 0;
	glDeleteShader(mGeometryShader); mGeometryShader = 0;
}
