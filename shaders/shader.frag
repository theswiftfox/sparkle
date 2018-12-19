#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) out vec4 outColor;

layout(location = 0) in vec3 normal;
layout(location = 1) in vec2 uv;

layout(set=1, binding = 0) uniform sampler2D diffuse;
layout(set=1, binding = 1) uniform sampler2D specular;

void main() {
	outColor = texture(diffuse, uv);
}