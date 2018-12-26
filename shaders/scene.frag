#version 450
#extension GL_ARB_separate_shader_objects : enable

#define SPARKLE_MAT_NORMAL_MAP 0x010
#define SPARKLE_MAT_PBR 0x100

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
	vec4 cameraPos;
	vec4 lightPos;
} ubo;

layout(push_constant) uniform MaterialProperties {
	uint features;
	float specular;
	float roughness;
	float metallic;
} mat;

// layout(set=0, binding = 3) uniform Lights {
//	todo dynamic lights
// } light;

vec4 blinnPhong(vec3 fragPos, vec3 normal, vec3 lightPos, vec3 viewPos) {
	vec4 diffColor = texture(diffuse, fs_in.uv);
	vec4 specColor = texture(specular, fs_in.uv);

	vec4 ambient = 0.05 * diffColor;

	// diffuse light
	vec3 L = normalize(lightPos - fragPos);
	vec3 N = normalize(normal);
	float diff = max(dot(L, N), 0.0);
	vec4 diffuse = diff * diffColor;

	// specular
	vec3 V = normalize(viewPos - fragPos);
	vec3 R = reflect(-L, N);
	vec3 halfDir = normalize(L + V);
	float spec = pow(max(dot(N, halfDir), 0.0), 16.0f);

	vec4 specular = spec * specColor * mat.specular;

	return (ambient + diffuse + specular);
}

void main() {
	vec3 normal;
	if ((mat.features & SPARKLE_MAT_NORMAL_MAP) == SPARKLE_MAT_NORMAL_MAP) {
		normal = texture(normalMap, fs_in.uv).rgb;
	} else {
		normal = fs_in.normal;
	}
/*	if ((mat.features & SPARKLE_MAT_PBR) == SPARKLE_MAT_PBR) {
		
	} else*/ {
		vec3 pos = fs_in.position; /*/fs_in.tangentPos; */
		vec3 light = ubo.lightPos.xyz; /*/fs_in.tangentLightPos; */
		vec3 view = ubo.cameraPos.xyz; /*/fs_in.tangentViewPos; */

		outColor = blinnPhong(pos, normal, light, view);
	}
}