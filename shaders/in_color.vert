#version 330

uniform mat4 modelViewProjMatrix;

in vec3 position;
in vec3 normal;
in vec4 vert_color;

out vec4 vs_color;
out vec3 vs_normal;

void main() {
	vs_color = vert_color;
	vs_normal = normal;
	gl_Position = vec4(position, 1.0);
}
