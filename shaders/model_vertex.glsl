#version 150

in vec3 position;
in  vec2 uv;

out vec2 out_uv;

uniform mat4 u_view_matrix;
uniform mat4 u_model_matrix;
uniform mat4 u_projection_matrix;

void main(){

	out_uv = uv;
	gl_Position =  u_projection_matrix * u_view_matrix * u_model_matrix * vec4(position, 1.0);

}