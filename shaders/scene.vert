#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location=0) in vec3 pos;
layout(location=1) in vec3 normal;
layout(location=2) in vec3 tangent;
layout(location=3) in vec3 bitangent;
layout(location=4) in vec2 uv;

out gl_PerVertex {
	vec4 gl_Position;
};

layout(location = 0) out VS_OUT {
	vec2 uv;
	vec3 normal;
	vec3 position;
	vec3 tangentPos;
	vec3 tangentViewPos;
	vec3 tangentLightPos;
} vs_out;

layout(binding=0) uniform UniformBufferObject {
	mat4 view;
	mat4 projection;
	vec4 cameraPos;
	vec4 lightPos;
} ubo;

layout(binding=1) uniform DynamicUniformBufferObject {
	mat4 model;
} dUbo;

void main() {
	vec4 posW = dUbo.model * vec4(pos, 1.0);
	gl_Position = ubo.projection * ubo.view * posW;

	vec3 T = normalize((dUbo.model * vec4(tangent, 1.0)).xyz);
	vec3 B = normalize((dUbo.model * vec4(bitangent, 1.0)).xyz);
	vec3 N = normalize((dUbo.model * vec4(normal, 1.0)).xyz);

	mat3 TBN = transpose(mat3(T, B, N));

	vs_out.uv = uv;
	vs_out.normal = normal;
	vs_out.position = posW.xyz;

	vs_out.tangentPos = TBN * posW.xyz;
	vs_out.tangentViewPos = TBN * ubo.cameraPos.xyz;
	vs_out.tangentLightPos = TBN * ubo.lightPos.xyz;

}