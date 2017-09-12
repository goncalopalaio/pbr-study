#version 150

in vec3 position;
in vec2 uv;
in vec3 normal;
in vec4 tangent;

uniform float u_time;
uniform vec3 u_light;
uniform vec3 u_camera_world;

uniform mat4 u_view_matrix;
uniform mat4 u_model_matrix;
uniform mat4 u_projection_matrix;

out vec2 _uv;


out vec3 view_dir_tan;
out vec3 light_dir_tan;

out vec3 temp_tangent;
void main(){
	gl_Position =  u_projection_matrix * u_view_matrix * u_model_matrix * vec4(position, 1.0);
	_uv = uv;

	temp_tangent = tangent.xyz;

	vec3 cam_pos_wor = (inverse(u_view_matrix) * vec4(0.0,0.0,0.0,1.0)).xyz;
	vec3 light_dir_wor = position - u_light;

	vec3 bitangent = cross(normal, tangent.xyz) * tangent.w;

	vec3 cam_pos_loc = vec3(inverse(u_model_matrix) * vec4(cam_pos_wor,1.0));

	vec3 light_dir_loc = vec3(inverse(u_model_matrix) * vec4(light_dir_wor, 0.0));

	vec3 view_dir_loc = normalize(cam_pos_loc - position);

	view_dir_tan = vec3(
		dot(tangent.xyz, view_dir_loc),
		dot(bitangent, view_dir_loc),
		dot(normal, view_dir_loc));

	light_dir_tan = vec3(
		dot(tangent.xyz, light_dir_loc),
		dot(bitangent, light_dir_loc),
		dot(normal, light_dir_loc));

}