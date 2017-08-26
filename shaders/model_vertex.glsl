#version 150

in vec3 position;
in vec2 uv;
in vec3 normal;
in vec4 tangent;

uniform float u_time;
uniform vec3 u_camera_world;

uniform mat4 u_view_matrix;
uniform mat4 u_model_matrix;
uniform mat4 u_projection_matrix;

out vec2 _uv;
out vec3 camera_world;
out vec3 world_pos;
out vec3 light_color;
out mat3 TBN;
out float time;

void main(){
	world_pos = position;
	light_color = vec3(1.0, 1.0, 1.0);
	camera_world = u_camera_world;
	time = u_time;

	vec3 bitangent = cross(normal, tangent.xyz) * tangent.w;

    TBN = mat3(tangent.xyz, bitangent, normal);
	_uv = uv;

	
	gl_Position =  u_projection_matrix * u_view_matrix * u_model_matrix * vec4(position, 1.0);
}