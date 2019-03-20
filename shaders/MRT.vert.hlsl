// Deferred Rendering vertex shader

struct VS_INPUT {
	[[vk::location(0)]] float3 position : POSITION;
	[[vk::location(1)]] float3 normal : NORMAL;
	[[vk::location(2)]] float3 tangent : TANGENT;
	[[vk::location(3)]] float3 bitangent : BITANGENT;
	[[vk::location(4)]] float2 uv : UV;
};

struct VS_OUTPUT {
	[[vk::location(0)]] float3 posWorld : POSITION_WORLD;
	[[vk::location(1)]] float3 normal : NORMAL;
	[[vk::location(2)]] float3 tangent : TANGENT;
	[[vk::location(3)]] float3 bitangent : BITANGENT;
	[[vk::location(4)]] float2 uv : UV;
};

[[vk::binding(0, 0)]] cbuffer ubo {
	float4x4 viewMat;
	float4x4 projectionMat;
}

[[vk::binding(1, 0)]] cbuffer dubo {
	float4x4 modelMat;
	float4x4 normalMat;
}

VS_OUTPUT main(in VS_INPUT input, out float4 vtxPos : SV_Position) {
	VS_OUTPUT output;
	float4 worldPos = mul(modelMat, float4(input.position, 1.0));
	output.posWorld = worldPos.xyz;

	//float3x3 normalMat = transpose(inverse(float3x3(model)));
	output.normal = mul(normalMat, normalize(float4(input.normal, 1.0))).xyz;
	output.tangent = mul(normalMat, normalize(float4(input.tangent, 1.0))).xyz;
	output.bitangent = input.bitangent;
	output.uv = input.uv;
	output.uv.t = 1.0 - output.uv.t;

	vtxPos = mul(projectionMat, mul(viewMat, worldPos));

	return output;
}
