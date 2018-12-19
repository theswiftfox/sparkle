#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location=0) in vec3 pos;
layout(location=1) in vec3 normal;
layout(location=2) in vec2 uv;

layout (location = 0) out vec2 fragUV;
layout (location = 1) out vec3 fragNormal;
layout (location = 2) out vec3 worldPos;

out gl_PerVertex {
	vec4 gl_Position;
};

layout(binding=0) uniform UniformBufferObject {
	mat4 view;
	mat4 projection;
} ubo;

layout(binding=1) uniform DynamicUniformBufferObject {
	mat4 model;
} dUbo;

void main() {
	vec4 posW = dUbo.model * vec4(pos, 1.0);
	gl_Position = ubo.projection * ubo.view * posW;
	worldPos = posW.xyz;
    fragNormal = normal;
	fragUV = uv;
}