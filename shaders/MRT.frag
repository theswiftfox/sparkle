#version 450
#extension GL_ARB_separate_shader_objects : enable

#define SPARKLE_MAT_NORMAL_MAP 0x010
#define SPARKLE_MAT_PBR 0x100

layout (location = 0) out vec4 outPosition;
layout (location = 1) out vec4 outNormal;
layout (location = 2) out uvec4 outAlbedoMR;

layout(location = 0) in VS_OUT {
	vec3 position;
	vec3 normal;
	vec3 tangent;
	vec3 bitangent;
	vec2 uv;

} fs_in;

layout(binding=0, set=1) uniform sampler2D albedoTexture;
layout(binding=0, set=1) uniform sampler2D specularTexture;
layout(binding=0, set=1) uniform sampler2D normalTexture;
layout(binding=0, set=1) uniform sampler2D roughnessTexture;
layout(binding=0, set=1) uniform sampler2D metallicTexture;

layout(constant_id = 0) const float NEAR_PLANE = 0.1f;
layout(constant_id = 1) const float FAR_PLANE = 1000.0f;
layout(constant_id = 2) const bool ENABLE_DISCARD = false;

layout(push_constant) uniform MaterialProperties {
	uint features;
} mat;

float calcLinearDepth(float zval)
{
	float z = zval * 2.0 - 1.0;
	return (2.0 * NEAR_PLANE * FAR_PLANE) / (FAR_PLANE + NEAR_PLANE - z * (FAR_PLANE - NEAR_PLANE));
}

void main() {
	outPosition = vec4(255.0, 0.0, 0.0, 1.0);
	outNormal = vec4(0.0, 0.0, 255.0, 1.0);
	outAlbedoMR = uvec4(0, 255, 0, 1);

	return;
	
	outPosition = texture(albedoTexture, fs_in.uv); //vec4(fs_in.position, calcLinearDepth(fs_in.position.z));
	vec4 albedo = texture(albedoTexture, fs_in.uv);

	vec4 normal = vec4(0.0);
	if ((mat.features & SPARKLE_MAT_NORMAL_MAP) == SPARKLE_MAT_NORMAL_MAP) {
		normal = texture(normalTexture, fs_in.uv);
	} else {
		normal = vec4(fs_in.normal, 0.0);
	}

//	if (ENABLE_DISCARD == false) {
//		vec3 N = normalize(normal).xyz;
///		vec3 B = normalize(fs_in.bitangent);
//		vec3 T = normalize(fs_in.tangent);
//		mat3 TBN = mat3(T, B, N);
//		vec3 n = TBN * normal.xyz;
//		outNormal = vec4(n * 0.5 + 0.5, 0.0);
//	} else {
		outNormal = vec4(normal.xyz * 0.5 + 0.5, 0.0);
//		if (albedo.a < 0.5) {
//			discard;
//		}
//	}

	// pack
	float metallic = texture(metallicTexture, fs_in.uv).r;
	float roughness = texture(roughnessTexture, fs_in.uv).r;

	outAlbedoMR.r = packHalf2x16(albedo.rg);
	outAlbedoMR.g = packHalf2x16(albedo.ba);

	if ((mat.features & SPARKLE_MAT_PBR) == SPARKLE_MAT_PBR) {
		outAlbedoMR.b = packHalf2x16(vec2(metallic, roughness));
		outAlbedoMR.a = 0;
	} else {
		vec4 specular = texture(specularTexture, fs_in.uv);
		outAlbedoMR.b = packHalf2x16(specular.rg);
		outAlbedoMR.a = packHalf2x16(specular.ba);
	}
}