#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) out vec4 outColor;

layout(location = 0) in VS_OUT {
	vec2 uv;
	vec3 normal;
	vec3 position;
	vec3 tangentPos;
	vec3 tangentViewPos;
	vec3 tangentLightPos;
} fs_in;

layout(set=1, binding = 0) uniform sampler2D diffuse;
layout(set=1, binding = 1) uniform sampler2D specular;
layout(set=1, binding = 2) uniform sampler2D normalMap;

layout(set=0, binding = 2) uniform FragUBO {
	vec3 cameraPos;
	vec3 lightPos;
} ubo;

// layout(set=0, binding = 3) uniform Lights {
//	todo dynamic lights
// } light;

vec3 blinnPhong(vec3 fragPos, vec3 normal, vec3 lightPos, vec3 viewPos) {
	vec3 diffColor = texture(diffuse, fs_in.uv).rgb;
	vec3 specColor = texture(specular, fs_in.uv).rgb;

	vec3 ambient = 0.05 * diffColor;

	// diffuse light
	vec3 L = normalize(lightPos - fragPos);
	vec3 N = normalize(normal);
	float diff = max(dot(L, N), 0.0);
	vec3 diffuse = diff * diffColor;

	// specular
	vec3 V = normalize(viewPos - fragPos);
	vec3 R = reflect(-L, N);
	vec3 halfDir = normalize(L + V);
	float spec = pow(max(dot(N, halfDir), 0.0), 32.0);

	vec3 specular = spec * specColor;

	return (ambient + diffuse + specular);
}

void main() {
	vec3 normal = /* fs_in.normal; */ texture(normalMap, fs_in.uv).rgb;
	vec3 pos = /*fs_in.position; */fs_in.tangentPos;
	vec3 light = /*ubo.lightPos; */fs_in.tangentLightPos;
	vec3 view = /*ubo.cameraPos; */fs_in.tangentViewPos;

	outColor = vec4(blinnPhong(pos, normal, light, view), 1.0);
}