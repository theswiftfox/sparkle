// Fullscreen Quad VTX Shader

struct VS_OUTPUT {
	[[vk::location(0)]] float2 uv : UV;
};

VS_OUTPUT main(uint id : SV_VertexId, out float4 vtxPos : SV_Position) {
	VS_OUTPUT output;
	output.uv = float2((id << 1) & 2, id & 2);
	vtxPos = float4(output.uv * 2 - 1, 0.0f, 1.0f);
	return output;
}