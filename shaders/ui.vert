#version 450
#extension GL_ARB_separate_shader_objects : enable

layout (location = 0) in vec2 position;
layout (location = 1) in vec2 uv;
layout (location = 2) in vec4 color;

layout (push_constant) uniform PushConstants {
	vec2 scale;
	vec2 translate;
} pushConstants;

layout (location = 0) out vec2 oUV;
layout (location = 1) out vec4 oColor;

out gl_PerVertex 
{
	vec4 gl_Position;   
};

void main() 
{
	oUV = uv;
	oColor = color;
	gl_Position = vec4(position * pushConstants.scale + pushConstants.translate, 0.0, 1.0);
}