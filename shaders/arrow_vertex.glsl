#version 150

in vec3 position;
in vec3 color;

uniform mat4 u_view_matrix;
uniform mat4 u_model_matrix;
uniform mat4 u_projection_matrix;

out vec3 _color;
void main(){	
	_color = color;
	gl_Position =  u_projection_matrix * u_view_matrix * u_model_matrix * vec4(position, 1.0);
}