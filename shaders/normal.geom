#version 330

uniform float thickness;
uniform float bump;

uniform mat4 modelViewProjMatrix;

layout(lines) in;
layout(triangle_strip, max_vertices = 4) out;

in vec4 vs_color[];

out vec4 gs_Color;

vec4 GetPosition(vec3 p, vec3 n, vec3 s) {
	// multiply by modelviewproj to get the actual projected vertex
	// p is the original point
	// add n * lambda to avoid z-fighting
	// add s * thickness to give width to the polygons
	return modelViewProjMatrix * vec4(p + n * 0.0001 + s * thickness, 1.0);
}

void main() {
	vec3 p1 = gl_in[0].gl_Position.xyz;
	vec3 p2 = gl_in[1].gl_Position.xyz;

	vec3 v = normalize(p2 - p1); 		// vector along the line
	vec3 n = vec3(1, 0, 0);				// normal vector
	vec3 s = cross(n, v);				// 'side' vector

	// create triangles
	gs_Color = vs_color[0];
	gl_Position = GetPosition(p1, n, +s);
	EmitVertex();
	gl_Position = GetPosition(p1, n, -s);
	EmitVertex();

	gs_Color = vs_color[1];
	gl_Position = GetPosition(p2, n, +s);
	EmitVertex();
	gl_Position = GetPosition(p2, n, -s);
	EmitVertex();
	
	EndPrimitive();
}
