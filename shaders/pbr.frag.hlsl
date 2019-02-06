// PBR based pixel shader

struct PS_INPUT {
	[[vk::location(0)]] float2 uv : UV;
};

struct PS_OUTPUT {
	[[vk::location(0)]] float4 color : SV_Target;
};

#define SPARKLE_SHADER_LIMIT_LIGHTS 9

#define SPARKLE_MAT_NORMAL_MAP 0x010
#define SPARKLE_MAT_PBR 0x100

[[vk::binding(0, 1)]] Texture2D positionTex;
[[vk::binding(1, 1)]] Texture2D normalsTex;
[[vk::binding(2, 1)]] Texture2D albedoMRTex;

SamplerState textureSampler {
	Filter = MIN_MAG_MIP_LINEAR;
	AddressU = Wrap;
	AddressV = Wrap;
};

struct Light {
	float4 position;
	float4 color;
	float radius;
};

[[vk::binding(2, 0)]] cbuffer ubo {
	float4 cameraPos;
	uint numberOfLights;
	float exposure;
	float gamma;
	Light[SPARKLE_SHADER_LIMIT_LIGHTS] lights;
};

struct MaterialProperties {
	uint features;
	float specular;
};
[[vk::constant_id(3)]] MaterialProperties mat;

const float PI = 3.14159265359;

float3 blinnPhong(float3 fragPos, float3 N, float3 V, float3 diffColor, float3 specColor, uint lightnr) {
	Light l = lights[lightnr];
	float3 lightPos = l.position.xyz;

	float d = length(lightPos - fragPos);
	float att = l.radius / ((d * d) + 1);
	float3 rad = l.color.rgb * att;

	// diffuse light
	float3 L = normalize(lightPos - fragPos);
	float diff = max(dot(L, N), 0.0);
	float3 diffuse = diff * diffColor * att;

	// specular
	float3 R = reflect(-L, N);
	float3 halfDir = normalize(L + V);
	float spec = pow(max(dot(N, halfDir), 0.0), 16.0f);

	float3 specular = spec * specColor * mat.specular * att;

	return (diffuse + specular);
}

// Normal Distribution function --------------------------------------
float NDF(float dotNH, float roughness)
{
	float alpha = roughness * roughness;
	float alpha2 = alpha * alpha;
	float denom = dotNH * dotNH * (alpha2 - 1.0) + 1.0;
	return (alpha2) / (PI * denom * denom);
}

// Geometric Distribution function --------------------------------------
float SchlickSmithGGX(float dotNL, float dotNV, float roughness)
{
	float r = (roughness + 1.0);
	float k = (r*r) / 8.0;
	float GL = dotNL / (dotNL * (1.0 - k) + k);
	float GV = dotNV / (dotNV * (1.0 - k) + k);
	return GL * GV;
}

// Fresnel function ----------------------------------------------------
vec3 FresnelSchlick(float cosTheta, float3 F0)
{
	float3 F = F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
	return F;
}

// Specular BRDF composition --------------------------------------------

vec3 BRDF(float3 V, float3 N, float3 albedo, float3 F0, Light light, float metallic, float roughness)
{
	// Precalculate vectors and dot products	
	float3 L = light.position.xyz - fs_in.position;
	float distance = length(L);
	float attenuation = 1.0 / (distance * distance);
	float3 radiance = light.color.rgb * attenuation;

	L = normalize(L);
	float3 H = normalize(V + L);

	float dotNV = clamp(dot(N, V), 0.0, 1.0);
	float dotNL = clamp(dot(N, L), 0.0, 1.0);
	float dotNH = clamp(dot(N, H), 0.0, 1.0);
	float dotHV = clamp(dot(L, H), 0.0, 1.0);

	float3 color = vec3(0.0);

	if (dotNL > 0.0 && dotNV > 0.0)
	{
		// D = Normal distribution
		float D = NDF(dotNH, roughness);
		// G = Geometric shadowing term (Microfacets shadowing)
		float G = SchlickSmithGGX(dotNL, dotNV, roughness);
		// F = Fresnel factor
		float3 F = FresnelSchlick(dotNV, F0);

		float3 nom = D * G * F;
		float denom = 4.0 * dotNV * dotNL;
		float3 spec = nom / denom;

		// energy conservation: the diffuse and specular light <= 1.0 (unless the surface emits light)
		// => diffuse component (kD) = 1.0 - kS.
		float3 kD = (vec3(1.0) - F);
		// only non metals have diffuse lightning -> linear blend with inverse metalness
		kD *= 1.0 - metallic;
		color += (kD * albedo / PI + spec) * radiance * dotNL;
	}

	return color;
}

// ----------------------------------------------------------------------------

PS_OUTPUT main(in PS_INPUT input) {
	m
}