#version 330

uniform mat4 modelViewProjMatrix;
uniform mat4 modelViewMatrix;
uniform mat3 normalMatrix;

in vec3 position;
in vec3 normal;

out vec3 N;
out vec3 V;

void main() {
	vec4 vertex = vec4(position, 1.0);

	// set up V and N for the fragment shader
	V = (modelViewMatrix * vertex).xyz;
	N = normalize(normalMatrix * normal);

	gl_Position = modelViewProjMatrix * vertex;
}
