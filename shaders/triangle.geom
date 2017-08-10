#version 330

uniform mat4 modelViewProjMatrix;

layout(triangles) in;
layout(triangle_strip, max_vertices = 3) out;

in vec4 vs_color[];
out vec4 gs_Color;

void main() {

	for (int i = 0; i < 3; ++i) {
		gs_Color = vs_color[i];
		gl_Position = modelViewProjMatrix * gl_in[i].gl_Position;
		EmitVertex();
	}
	EndPrimitive();
}
