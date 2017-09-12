#version 150

in vec2 _uv;
in vec3 view_dir_tan;
in vec3 light_dir_tan;

in vec3 temp_tangent;

out vec4 frag_color;

uniform sampler2D u_texture;
uniform sampler2D u_albedoMap;
uniform sampler2D u_normalMap;
uniform sampler2D u_metallicMap;
uniform sampler2D u_roughnessMap;
uniform sampler2D u_aoMap;

float PI = 3.14159265359;

void main() {
//vec3 the_normal = 0.5 + 0.5*texture(u_normalMap, _uv).rgb;

	vec3 Ia = vec3(0.2, 0.2, 0.2);
	
	vec3 normal_tan = texture(u_normalMap, _uv).rgb;
	normal_tan = normalize(normal_tan * 2.0 - 1.0);

	vec3 direction_to_light_tan = normalize(-light_dir_tan);
	float dot_prod = dot(direction_to_light_tan, normal_tan);
	dot_prod = max(dot_prod, 0.0);

	vec3 Id = vec3(0.7,0.7,0.7) * vec3(1.0, 0.5, 0.0) * dot_prod;

	vec3 reflection_tan = reflect(normalize(light_dir_tan), normal_tan);

	float dot_prod_specular = dot(reflection_tan, normalize(view_dir_tan));
	dot_prod_specular = max(dot_prod_specular, 0.0);

	float specular_factor = pow(dot_prod_specular, 100.0);

	vec3 Is = vec3(1.0,1.0,1.0) * vec3(0.5,0.5,0.5) * specular_factor;

	vec3 c  = Is + Id + Ia;
	c = texture(u_albedoMap, _uv).rgb * c;
	


	frag_color = vec4(c, 1.0);
	//frag_color = vec4(temp_tangent, 1.0);

//frag_color = texture(u_texture, _uv);
//frag_color = texture(u_albedoMap, _uv);
//frag_color = texture(u_normalMap, _uv);
//frag_color = texture(u_metallicMap, _uv);
//frag_color = texture(u_roughnessMap, _uv);
//frag_color = texture(u_aoMap, _uv);
}