#version 330

uniform mat4 modelViewProjMatrix;

in vec3 position;
in vec3 normal;

out vec4 gs_Color;

void main() {
	vec4 vertex = vec4(position, 1.0);
	gs_Color = vec4(0.4, 0.4, 0.4, 1.0);
	gl_Position = modelViewProjMatrix * vertex;
}
