#version 330

in vec4 gs_Color;
out vec4 fs_Color;

void main() {
	fs_Color = gs_Color;
}
