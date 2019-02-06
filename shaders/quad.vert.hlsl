// Fullscreen Quad VTX Shader

struct VS_INPUT {
	uint vertexId : SV_VertexID;
};

struct VS_OUTPUT {
	[[vk::location(0)]] float2 uv : UV;
};

VS_OUTPUT main(in VS_INPUT input, out float4 vtxPos : SV_Position) {
	VS_OUTPUT output;
	output.uv = float2((input.vertexId << 1) & 2, input.vertexId & 2);
	vtxPos = float4(output.uv * 2.0f - 1.0f, 0.0f, 1.0f);
}