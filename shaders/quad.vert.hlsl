// Fullscreen Quad VTX Shader

struct VS_INPUT {
	[[vk::location(0)]] float3 position : POSITION;
	[[vk::location(1)]] float3 normal : NORMAL;
	[[vk::location(2)]] float3 tangent : TANGENT;
	[[vk::location(3)]] float3 bitangent : BITANGENT;
	[[vk::location(4)]] float2 uv : UV;
};