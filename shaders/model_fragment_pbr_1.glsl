#version 150


in vec2 _uv;
in vec3 view_dir_tan;
in vec3 light_dir_tan;

out vec4 frag_color;

uniform sampler2D u_texture;
uniform sampler2D u_albedoMap;
uniform sampler2D u_normalMap;
uniform sampler2D u_metallicMap;
uniform sampler2D u_roughnessMap;
uniform sampler2D u_aoMap;

float PI = 3.14159265359;

float DistribuitionGGX(vec3 N, vec3 H, float roughness);
float GeometrySchlickGGX(float NdotV, float roughness);
float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness);
vec3 fresnelSchlick(float cosTheta, vec3 F0);
vec3 pow_v(vec3 v, float val);

void main() {
	vec3 light_color = vec3(1.0,1.0,1.0);
	vec3 albedo = pow_v(texture(u_albedoMap, _uv).rgb, 2.2);
	float metallic = texture(u_metallicMap, _uv).r;
	float roughness = texture(u_roughnessMap, _uv).r;
	float ao = 0.0;//texture(u_aoMap, _uv).r;
	
	vec3 normal = texture (u_normalMap, _uv).rgb;
	normal = normalize (normal * 2.0 - 1.0);



	vec3 N = normal; 
	vec3 V = normalize(view_dir_tan);


	vec3 F0 = vec3(0.04);
	F0 = mix(F0, albedo, metallic);

	vec3 Lo = vec3(0.0);
	// start loop lights
	

	vec3 L = normalize(light_dir_tan);
	vec3 H = normalize(V + L);
	float distance = length(light_dir_tan);
	// calculate per-light radiance
	float attenuation = 1.0 / (distance * distance);
	vec3 radiance = light_color * attenuation;

	// cook torrance brdf
	float NDF = DistribuitionGGX(N,H, roughness);
	float G = GeometrySmith(N,V,L, roughness);
	vec3 F = fresnelSchlick(max(dot(H,V),0.0), F0);


		vec3 kS = F;
		vec3 kD = vec3(1.0) - kS;
		kD *= 1.0 - metallic;

		vec3 nominator = NDF * G * F;
		float denominator = 4 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.001;
		vec3 specular = nominator / denominator;

		// add to outgoing radiance Lo
		float NdotL = max(dot(N,L),0.0);
		Lo += (kD * albedo / PI + specular) * radiance * NdotL;

	// end loop lights


	vec3 ambient = vec3(0.09) * albedo * ao;
	vec3 color = ambient + Lo;

	color = color/ (color + vec3(1.0));
	color = pow(color, vec3(1.0/2.2));


	frag_color = vec4(color, 1.0);
	/*

frag_color = vec4(color, 1.0);
*/
//frag_color = texture(u_texture, _uv);
//frag_color = texture(u_albedoMap, _uv);
//frag_color = texture(u_normalMap, _uv);
//frag_color = texture(u_metallicMap, _uv);
//frag_color = texture(u_roughnessMap, _uv);
//frag_color = texture(u_aoMap, _uv);
}


float DistribuitionGGX(vec3 N, vec3 H, float roughness) {
	float a = roughness * roughness;
	float a2 = a*a;
	float NdotH = max(dot(N,H), 0.0);
	float NdotH2 = NdotH * NdotH;

	float nom = a2;
	float denom = (NdotH2 * (a2 - 1.0) + 1.0);
	denom = PI * denom * denom;

	return nom / denom;
}

float GeometrySchlickGGX(float NdotV, float roughness) {
	float r = (roughness + 1.0);
	float k = (r*r) / 8.0;

	float nom = NdotV;
	float denom = NdotV * (1.0 - k) + k;
	return nom / denom;
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness) {
	float NdotV = max(dot(N, V), 0.0);
	float NdotL = max(dot(N, L), 0.0);
	float ggx2 = GeometrySchlickGGX(NdotV, roughness);
	float ggx1 = GeometrySchlickGGX(NdotL, roughness);
	return ggx1 * ggx2;
}

vec3 fresnelSchlick(float cosTheta, vec3 F0) {
	return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}

vec3 pow_v(vec3 v, float val) {
	return vec3(pow(v.x,val), pow(v.y,val), pow(v.z,val));
}