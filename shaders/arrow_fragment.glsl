#version 150
in vec3 _color;

out vec4 frag_color;
void main() {
	frag_color = vec4(_color,1.0);
}