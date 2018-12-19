#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) out vec4 outColor;

layout(location = 0) in vec2 UV;
layout(location = 1) in vec3 Normal;
layout(location = 2) in vec3 WorldPos;

layout(set=1, binding = 0) uniform sampler2D diffuse;
layout(set=1, binding = 1) uniform sampler2D specular;

layout(set=0, binding = 2) uniform FragUBO {
	vec3 cameraPos;
	vec3 lightPos;
} ubo;

// layout(set=0, binding = 3) uniform Lights {
//	todo dynamic lights
// } light;

void main() {
	vec3 diffColor = texture(diffuse, UV).rgb;
	vec3 specColor = texture(specular, UV).rgb;

	vec3 ambient = 0.05 * diffColor;

	// diffuse light
	vec3 L = normalize(ubo.lightPos - WorldPos);
	vec3 N = normalize(Normal);
	float diff = max(dot(L, N), 0.0);
	vec3 diffuse = diff * diffColor;

	// specular
	vec3 V = normalize(ubo.cameraPos - WorldPos);
	vec3 R = reflect(-L, N);
	vec3 halfDir = normalize(L + V);
	float spec = pow(max(dot(N, halfDir), 0.0), 32.0);

	vec3 specular = spec * specColor;

	outColor = vec4(ambient + diffuse + specular, 1.0);
}