// Deferred rendering pixel shader

#define SPARKLE_MAT_NORMAL_MAP 0x010
#define SPARKLE_MAT_PBR 0x100

struct PS_INPUT {
	[[vk::location(0)]] float3 posWorld : POSITION_WORLD;
	[[vk::location(1)]] float3 normal : NORMAL;
	[[vk::location(2)]] float3 tangent : TANGENT;
	[[vk::location(3)]] float3 bitangent : BITANGENT;
	[[vk::location(4)]] float2 uv : UV;
};

struct PS_OUTPUT {
	[[vk::location(0)]] float4 position;
	[[vk::location(1)]] float4 normal;
	[[vk::location(2)]] float4 albedo;
	[[vk::location(3)]] float4 pbrSpecular;
};

[[vk::binding(0, 1)]] Texture2D albedoTexture;
[[vk::binding(1, 1)]] Texture2D specularTexture;
[[vk::binding(2, 1)]] Texture2D normalTexture;
[[vk::binding(3, 1)]] Texture2D roughnessTexture;
[[vk::binding(4, 1)]] Texture2D metallicTexture;

[[vk::constant_id(0)]] const float NEAR_PLANE = 0.1f;
[[vk::constant_id(1)]] const float FAR_PLANE = 1000.0f;
[[vk::constant_id(2)]] const bool ENABLE_DISCARD = false;

[[vk::push_constant]] cbuffer mat
{
	uint materialFeatures;
};

float calcLinearDepth(float zval)
{
	float z = zval * 2.0 - 1.0;
	return (2.0 * NEAR_PLANE * FAR_PLANE) / (FAR_PLANE + NEAR_PLANE - z * (FAR_PLANE - NEAR_PLANE));
}

[[vk::location(0)]] PS_OUTPUT main(PS_INPUT input, in float4 pos
                                   : SV_Position)
{
	PS_OUTPUT output;
	output.position = float4(input.posWorld, calcLinearDepth(pos.z));

	SamplerState textureSampler
	{
		Filter = MIN_MAG_MIP_LINEAR;
		AddressU = Wrap;
		AddressV = Wrap;
	};

	float4 albedo = albedoTexture.Sample(textureSampler, input.uv);
	float4 normal;
	if ((materialFeatures & SPARKLE_MAT_NORMAL_MAP) == SPARKLE_MAT_NORMAL_MAP) {
		normal = normalTexture.Sample(textureSampler, input.uv);
	} else {
		normal = float4(input.normal, 0.0);
	}

	if (ENABLE_DISCARD == false) {
		float3 N = normalize(input.normal);
		float3 B = normalize(input.bitangent);
		float3 T = normalize(input.tangent);
		float3x3 TBN = float3x3(T, B, N);
		float3 n = normal.xyz * 2.0 - float3(1.0);
		n = mul(normalize(n), TBN);
		output.normal = float4(n * 0.5 + 0.5, 0.0);
	} else {
		output.normal = float4(input.normal * 0.5 + 0.5, 0.0);
		if (albedo.a < 0.5) {
			discard;
		}
	}

	// packing - not working, idk. i'm stupid? maybe use clear value of uint32_t?
	//uint4 chalf = f32tof16(albedo);
	//output.albedoMR.r = (chalf.r << 8) | chalf.g;
	//output.albedoMR.g = (chalf.b << 8) | chalf.a;
	//if (materialFeatures & SPARKLE_MAT_PBR == SPARKLE_MAT_PBR) {
	//	float roughness = roughnessTexture.Sample(textureSampler, input.uv).r;
	//	float metallic = metallicTexture.Sample(textureSampler, input.uv).r;
	//	uint rhalf = f32tof16(roughness);
	//	uint mhalf = f32tof16(metallic);

	//	output.albedoMR.b = (mhalf << 8) | rhalf;
	//	output.albedoMR.a = 0;
	//} else {
	//	float4 specular = specularTexture.Sample(textureSampler, input.uv);
	//	uint4 shalf = f32tof16(specular);
	//	output.albedoMR.b = (shalf.r << 8) | shalf.g;
	//	output.albedoMR.a = (shalf.b << 8) | shalf.a;
	//}

	//return output;

	output.albedo = albedo;
	if ((materialFeatures & SPARKLE_MAT_PBR) == SPARKLE_MAT_PBR) {
		float roughness = roughnessTexture.Sample(textureSampler, input.uv).r;
		float metallic = metallicTexture.Sample(textureSampler, input.uv).r;
		output.pbrSpecular = float4(metallic, roughness, 0.0, 0.0);
	} else {
		output.pbrSpecular = specularTexture.Sample(textureSampler, input.uv);
		output.pbrSpecular.a = 1.0;
	}

	return output;
}