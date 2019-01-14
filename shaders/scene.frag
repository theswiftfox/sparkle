#version 450
#extension GL_ARB_separate_shader_objects : enable

#define SPARKLE_SHADER_LIMIT_LIGHTS 9

#define SPARKLE_MAT_NORMAL_MAP 0x010
#define SPARKLE_MAT_PBR 0x100

layout(location = 0) out vec4 outColor;

layout(location = 0) in VS_OUT {
	vec2 uv;
	vec3 normal;
	vec3 position;
//	vec3 tangentPos;
//	vec3 tangentViewPos;
//	vec3 tangentLightPos;
} fs_in;

layout(set=1, binding = 0) uniform sampler2D diffuse;
layout(set=1, binding = 1) uniform sampler2D specular;
layout(set=1, binding = 2) uniform sampler2D normalMap;
layout(set=1, binding = 3) uniform sampler2D roughnessMap;
layout(set=1, binding = 4) uniform sampler2D metallicMap;

struct Light {
	vec4 position;
	vec4 color;
	float radius;
};

layout(std140, set=0, binding = 2) uniform FragUBO {
	vec4 cameraPos;
	uint numberOfLights;
	float exposure;
	float gamma;
	Light[SPARKLE_SHADER_LIMIT_LIGHTS] lights;
} ubo;

layout(push_constant) uniform MaterialProperties {
	uint features;
	float specular;
} mat;

const float PI = 3.14159265359;

// from https://learnopengl.com/code_viewer_gh.php?code=src/6.pbr/1.2.lighting_textured/1.2.pbr.fs
vec3 getNormalFromMap()
{
    vec3 tangentNormal = texture(normalMap, fs_in.uv).xyz * 2.0 - 1.0;

    vec3 Q1  = dFdx(fs_in.position);
    vec3 Q2  = dFdy(fs_in.position);
    vec2 st1 = dFdx(fs_in.uv);
    vec2 st2 = dFdy(fs_in.uv);

    vec3 N   = normalize(fs_in.normal);
    vec3 T  = normalize(Q1*st2.t - Q2*st1.t);
    vec3 B  = -normalize(cross(N, T));
    mat3 TBN = mat3(T, B, N);

    return normalize(TBN * tangentNormal);
}

vec3 blinnPhong(vec3 fragPos, vec3 N, vec3 V, vec3 diffColor, vec3 specColor, int lightnr) {
	Light l = ubo.lights[lightnr];
	vec3 lightPos = l.position.xyz;

	float d = length(lightPos - fragPos);
	float att = l.radius / ((d * d) + 1);
	vec3 rad = l.color.rgb * att;

	// diffuse light
	vec3 L = normalize(lightPos - fragPos);
	float diff = max(dot(L, N), 0.0);
	vec3 diffuse = diff * diffColor * att;

	// specular
	vec3 R = reflect(-L, N);
	vec3 halfDir = normalize(L + V);
	float spec = pow(max(dot(N, halfDir), 0.0), 16.0f);

	vec3 specular = spec * specColor * mat.specular * att;

	return (diffuse + specular);
}

// Normal Distribution function --------------------------------------
float NDF(float dotNH, float roughness)
{
	float alpha = roughness * roughness;
	float alpha2 = alpha * alpha;
	float denom = dotNH * dotNH * (alpha2 - 1.0) + 1.0;
	return (alpha2)/(PI * denom * denom); 
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
vec3 FresnelSchlick(float cosTheta, vec3 F0)
{
	vec3 F = F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0); 
	return F;    
}

// Specular BRDF composition --------------------------------------------

vec3 BRDF(vec3 V, vec3 N, vec3 albedo, vec3 F0, Light light, float metallic, float roughness)
{
	// Precalculate vectors and dot products	
	vec3 L = light.position.xyz - fs_in.position;
	float distance = length(L);
	float attenuation = 1.0 / (distance * distance);
	vec3 radiance = light.color.rgb * attenuation;

	L = normalize(L);
	vec3 H = normalize (V + L);

	float dotNV = clamp(dot(N, V), 0.0, 1.0);
	float dotNL = clamp(dot(N, L), 0.0, 1.0);
	float dotNH = clamp(dot(N, H), 0.0, 1.0);
	float dotHV = clamp(dot(L, H), 0.0, 1.0);

	vec3 color = vec3(0.0);

	if (dotNL > 0.0 && dotNV > 0.0)
	{
		// D = Normal distribution
		float D = NDF(dotNH, roughness); 
		// G = Geometric shadowing term (Microfacets shadowing)
		float G = SchlickSmithGGX(dotNL, dotNV, roughness);
		// F = Fresnel factor
		vec3 F = FresnelSchlick(dotNV, F0);

		vec3 nom = D * G * F;
		float denom = 4.0 * dotNV * dotNL;
		vec3 spec = nom / denom;

		// energy conservation: the diffuse and specular light <= 1.0 (unless the surface emits light)
        // => diffuse component (kD) = 1.0 - kS.
		vec3 kD = (vec3(1.0) - F);
		// only non metals have diffuse lightning -> linear blend with inverse metalness
		kD *= 1.0 - metallic;
		color += (kD * albedo / PI + spec) * radiance * dotNL;
	}

	return color;
}

// ----------------------------------------------------------------------------

void main() {
	vec3 N;
	if ((mat.features & SPARKLE_MAT_NORMAL_MAP) == SPARKLE_MAT_NORMAL_MAP) {
		N = getNormalFromMap(); //texture(normalMap, fs_in.uv).rgb;
	} else {
		vec3 normal = fs_in.normal;
		N = normalize(normal);
	}
	vec3 pos = fs_in.position; /*/fs_in.tangentPos; */
	vec3 view = ubo.cameraPos.xyz; /*/fs_in.tangentViewPos; */

	vec3 V = normalize(view - pos);
	if ((mat.features & SPARKLE_MAT_PBR) == SPARKLE_MAT_PBR) {
		vec3 albedo = pow(texture(diffuse, fs_in.uv).rgb, vec3(2.2));
		float roughness = texture(roughnessMap, fs_in.uv).r;
		float metallic = texture(metallicMap, fs_in.uv).r;

		vec3 F0 = mix(vec3(0.04), albedo, metallic);
		
		vec3 lo;
		for (int i = 0; i < 9; i++) {
			lo += BRDF(V, N, albedo, F0, ubo.lights[i], metallic, roughness);
		}
		vec3 color = albedo * 0.03 + lo;
		
		color = vec3(1.0) - exp(-color * ubo.exposure);
		color = pow(color, vec3(1.0 / ubo.gamma));
		
		outColor = vec4(color, 1.0);
	} else {
		vec3 diffColor = texture(diffuse, fs_in.uv).rgb;
		vec3 specColor = texture(specular, fs_in.uv).rgb;

		vec3 lo;
		for (int i = 0; i < SPARKLE_SHADER_LIMIT_LIGHTS; ++i) {
			lo += blinnPhong(pos, N, V, diffColor, specColor, i);
		}
		vec3 color = vec3(0.03) * texture(diffuse, fs_in.uv).rgb + lo;

		color = vec3(1.0) - exp(-color * ubo.exposure);
		color = pow(color, vec3(1.0 / ubo.gamma));
		
		outColor = vec4(color, 1.0);
	}
}