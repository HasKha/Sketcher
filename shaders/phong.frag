#version 330

uniform mat3 viewMatrix = mat3(1.0);

uniform float ambient_intensity = 0.3;
uniform float diffuse_intensity = 1.0;
uniform float specular_intensity = 1.0;
uniform float shininess = 15.0;

// using the same color for all lights
const vec3 LightColor = vec3(1, 1, 1);

uniform vec3 l0Position = vec3(6, 3, 3); // key light
uniform float l0Intensity = 1.0;

uniform vec3 l1Position = vec3(6, 0, -3); // back light
uniform float l1Intensity = 0.5;

uniform vec3 l2Position = vec3(-6, 0, 2); // fill light
uniform float l2Intensity = 0.7;

// const vec3 MaterialDiffuseColor = vec3(0.34, 0.61, 0.74);
uniform vec3 DiffuseMaterial = vec3(0.6, 0.5, 0.4);
uniform vec3 AmbientMaterial = vec3(0.2, 0.2, 0.2);
uniform vec3 SpecularMaterial = vec3(0.3, 0.2, 0.1);

const vec4 l0dif = vec4(1.0, 1.0, 1.0, 1.0);
const vec4 l0spe = vec4(1.0, 1.0, 1.0, 1.0);

in vec3 N;
in vec3 V;

out vec4 color;

void main() {
	// normalize the normal
	vec3 norm = normalize(N);

	// distance to the light
	// float l0dist = length(l0Position - V);

	// direction of the light (from fragment to light source)
	// note: lights move with the camera, so we multiply light 
	// positions by view matrix
	vec3 l0L = normalize(viewMatrix * l0Position - V);
	vec3 l1L = normalize(viewMatrix * l1Position - V);
	vec3 l2L = normalize(viewMatrix * l2Position - V);
	
	// === ambient term ===
	vec3 Ambient = AmbientMaterial * ambient_intensity;

	// === diffuse term ===
	// cosine of angle between normal and light direction, clamped to [0, 1]
	// light is at the vertical -> 1
	// light is perpendicular -> 0
	// light is behind -> 0
	float l0cosThetha = clamp(dot(norm, l0L), 0, 1);
	float l1cosThetha = clamp(dot(norm, l1L), 0, 1);
	float l2cosThetha = clamp(dot(norm, l2L), 0, 1);
	vec3 l0Diffuse = diffuse_intensity * DiffuseMaterial * LightColor * l0Intensity * l0cosThetha;
	vec3 l1Diffuse = diffuse_intensity * DiffuseMaterial * LightColor * l1Intensity * l1cosThetha;
	vec3 l2Diffuse = diffuse_intensity * DiffuseMaterial * LightColor * l2Intensity * l2cosThetha;
	vec3 Diffuse = l0Diffuse + l1Diffuse + l2Diffuse;

	// === specular term ===
	// reflected direction
	vec3 l0R = reflect(-l0L, norm);

	// eye vector
	vec3 E = normalize(-V);

	// cosine of angle between eye vector and reflect vector, clamped to [0, 1]
	// looking into reflection -> 1
	// looking elsewhere -> [0, 1]
	float cosAlpha = clamp(dot(E, l0R), 0, 1);

	vec3 Specular = specular_intensity * SpecularMaterial * LightColor * l0Intensity * pow(cosAlpha, shininess);// / (dist * dist);

	// combine lights
	color = vec4(clamp(Ambient + Diffuse + Specular, 0, 1), 1);
}
