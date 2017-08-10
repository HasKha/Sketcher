#version 330

uniform mat4 modelViewProjMatrix;
uniform vec4 vert_color;

in vec3 position;
in vec3 normal;

out vec3 vs_normal;

void main() {
	vs_normal = normal;
	gl_Position = modelViewProjMatrix * vec4(position, 1.0);
}
