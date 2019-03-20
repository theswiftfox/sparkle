#version 450
#extension GL_ARB_separate_shader_objects : enable

// Deferred Rendering vertex shader

layout(location=0) in vec3 position;
layout(location=1) in vec3 normal;
layout(location=2) in vec3 tangent;
layout(location=3) in vec3 bitangent;
layout(location=4) in vec2 uv;

out gl_PerVertex {
	vec4 gl_Position;
};

layout(location = 0) out VS_OUT {
	vec3 position;
	vec3 normal;
	vec3 tangent;
	vec3 bitangent;
	vec2 uv;

} vs_out;

layout(binding=0, set=0) uniform UBO {
	mat4 viewMat;
	mat4 projMat;
} ubo;

layout(binding=1, set=0) uniform dUBO {
	mat4 modelMat;
	mat4 normalMat;
} dUbo;

void main() {
	vec4 worldPos = dUbo.modelMat * vec4(position, 1.0);
	vs_out.position = worldPos.xyz;
	vs_out.normal = (dUbo.normalMat * normalize(vec4(normal, 1.0))).xyz;
	vs_out.tangent = (dUbo.normalMat * normalize(vec4(tangent, 1.0))).xzy;
	vs_out.bitangent = bitangent;
	vs_out.uv = uv;
	vs_out.uv.t = 1.0 - vs_out.uv.t;

	gl_Position = ubo.projMat * ubo.viewMat * worldPos;

}