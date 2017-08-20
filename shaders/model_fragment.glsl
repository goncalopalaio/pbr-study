#version 150
in vec2 out_uv;
out vec4 frag_color;
uniform sampler2D u_texture;
void main() {
	frag_color = texture(u_texture, out_uv) * vec4(1.0,1.0,1.0,1.0);
}