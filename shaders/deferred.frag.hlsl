// PBR based pixel shader

struct PS_INPUT {
	[[vk::location(0)]] float2 uv : UV;
};

struct PS_OUTPUT {
	[[vk::location(0)]] float4 color : SV_Target;
};

#define MASK 0x0000FFFF

#define SPARKLE_SHADER_LIMIT_LIGHTS 1000

#define SPARKLE_MAT_NORMAL_MAP 0x010
#define SPARKLE_MAT_PBR 0x100

[[vk::binding(1, 0)]] Texture2D positionTex;
[[vk::binding(2, 0)]] Texture2D normalsTex;
[[vk::binding(3, 0)]] Texture2D albedoTex;
[[vk::binding(4, 0)]] Texture2D pbrSpecularTex;

struct Light {
	float4 position;
	float4 color;
	float radius;
};

[[vk::binding(0, 0)]] cbuffer ubo
{
	float4 cameraPos;
	uint numberOfLights;
	float exposure;
	float gamma;
	Light lights[SPARKLE_SHADER_LIMIT_LIGHTS];
};

static const float PI = 3.14159265359;
static const float Epsilon = 0.001;
static const float MinRoughness = 0.04;

float3 blinnPhong(float3 fragPos, float3 N, float3 V, float3 diffColor, float3 specColor, uint lightnr)
{
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

	float3 specular = spec * specColor * att;

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
	float k = (r * r) / 8.0;
	float GL = dotNL / (dotNL * (1.0 - k) + k);
	float GV = dotNV / (dotNV * (1.0 - k) + k);
	return GL * GV;
}

// Fresnel function ----------------------------------------------------
float3 FresnelSchlick(float cosTheta, float3 F0)
{
	float3 F = F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
	return F;
}

// Specular BRDF composition --------------------------------------------
float3 BRDF(float3 V, float3 N, float3 position, float3 albedo, float3 F0, Light light, float metallic, float roughness)
{
	// Precalculate vectors and dot products
	float3 L = light.position.xyz - position;
	float distance = length(L);
	float attenuation = 1.0 / (distance * distance);
	float3 radiance = light.color.rgb * attenuation;

	L = normalize(L);
	float3 H = normalize(L + V);

	float dotNV = clamp(abs(dot(N, V)), 0.001, 1.0);
	float dotNL = clamp(dot(N, L), 0.001, 1.0);
	float dotNH = clamp(dot(N, H), 0.0, 1.0);
	float dotHV = clamp(dot(L, H), 0.0, 1.0);
	//float dotNV = max(dot(N, V), 0.0);
	//float dotNL = max(dot(N, L), 0.0);
	//float dotNH = max(dot(N, H), 0.0);
	//float dotHV = max(dot(H, V), 0.0);

	radiance *= dotNL;

	float3 color = 0.0;

//	if (dotNL > 0.0/* && dotNV > 0.0*/) {
		// D = Normal distribution
		float D = NDF(dotNH, roughness);
		// G = Geometric shadowing term (Microfacets shadowing)
		float G = SchlickSmithGGX(dotNL, dotNV, roughness);
		// F = Fresnel factor
		float3 F = FresnelSchlick(dotHV, F0);

		//float3 kD = lerp(float3(1, 1, 1) - F, float3(0, 0, 0), metallic);
		float3 kD = 1.0 - F;
		float3 diffuse = kD * (albedo / PI);
		float3 specular = (F * G * D) / (4.0 * dotNL * dotNV);

		color += (diffuse + specular) * radiance;

		//float3 nom = D * G * F;
		//float denom = max(4.0 * dotNV * dotNL, Epsilon);
		//float3 spec = nom / denom;

		//// energy conservation: the diffuse and specular light <= 1.0 (unless the surface emits light)
		//// => diffuse component (kD) = 1.0 - kS.
		//float3 kD = (float3(1.0, 1.0, 1.0) - F);
		//// only non metals have diffuse lightning -> linear blend with inverse metalness
		//kD *= 1.0 - metallic;
		//color += (kD * albedo / PI + spec) * radiance;
//	}

	return color;
}

// ----------------------------------------------------------------------------

PS_OUTPUT main(in PS_INPUT input)
{
	PS_OUTPUT output;

	SamplerState textureSampler
	{
		Filter = MIN_MAG_MIP_LINEAR;
		AddressU = ClampToEdge;
		AddressV = ClampToEdge;
	};
	// unpack
	float4 pos = positionTex.Sample(textureSampler, input.uv);

	if (length(pos.rgb) == 0.0) {
		output.color = 0.0;
		return output;
	}

	float3 normal = normalsTex.Sample(textureSampler, input.uv).rgb * 2.0 - 1.0;

	float4 albedo = albedoTex.Sample(textureSampler, input.uv);
	float4 specularPbr = pbrSpecularTex.Sample(textureSampler, input.uv);

	float3 color = 0.0;

	float3 V = normalize(cameraPos.rgb - pos.rgb);
	float3 N = normalize(normal);

	if (specularPbr.a < 1.0) { // use pbr rendering TODO: use a better switch instead of alpha value
		float metallic = specularPbr.r;
		float roughness = clamp(specularPbr.g, MinRoughness, 1.0);

		float3 lo = 0.0;
		float3 F0 = lerp(float3(0.04, 0.04, 0.04), albedo.rgb, metallic);

		for (uint i = 0; i < numberOfLights; i++) {
			lo += BRDF(V, N, pos.rgb, albedo.rgb, F0, lights[i], metallic, roughness);
		}
		//color = float3(0.03, 0.03, 0.03) * albedo + lo;
		color = lo;
	} else {
		float3 lo = float3(0.0, 0.0, 0.0);
		for (uint i = 0; i < numberOfLights; i++) {
			lo += blinnPhong(pos.rgb, N, V, albedo, specularPbr.rgb, i);
		}
		color = float3(0.03, 0.03, 0.03) * albedo + lo;
	}

	color = float3(1.0, 1.0, 1.0) - exp(-color * exposure);
	float gammaDiv = 1.0 / gamma;
	color = pow(color, float3(gammaDiv, gammaDiv, gammaDiv));
	output.color = float4(color, albedo.a);

	return output;
}