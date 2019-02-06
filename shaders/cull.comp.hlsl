//--------------------------------------------------------------------------------------
// File: cull.comp.hlsl
//
// This file contains the Compute Shader to perform occlusion culling
// 
// Copyright (c) Patrick Gantner. All rights reserved.
//--------------------------------------------------------------------------------------

struct InstanceData {
	float4x4 model;
	uint firstIndex;
	uint indexCount;
};

[[vk::binding(0)]] cbuffer Instances {
	InstanceData instances[];
};

struct DrawCommandIndexIndirect {
	uint indexCount;
	uint instanceCount;
	uint firstIndex;
	uint vertexOffset;
	uint firstInstance;
};

[[vk::binding(1)]] RWStructuredBuffer<DrawCommandIndexIndirect> indirectDraws;

[[vk::binding(2)]] cbuffer ubo {
	float4x4 viewMat;
	float4x4 projectionMat;
	float4 cameraPos;
	float4 frustumCube[6];
}

[[vk::binding(3)]] cbuffer uboOut {
	uint drawCount;
};

bool checkFrustum(float4 pos, float rad) {
	for (uint i = 0; i < 6; ++i) {
		if (dot(pos, frustumCube[i]) + rad < 0.0) {
			return false;
		}
	}
	return true;
}

[[vk::constant_id(0)]] const uint workGroupSize = 1;
[[vk::constant_id(1)]] const uint numWorkGroups = 1;


void main(uint3 DTid : SV_DispatchThreadID) {
	uint idx = DTid.x + DTid.y * numWorkGroups * workGroupSize;

	if (idx == 0) {
		uint orig;
		InterlockedExchange(drawCount, 0, orig);
	}

	float4x4 iModel = instances[idx].model;
	float4 pos = float4(iModel._41, iModel._42, iModel._43, 1.0f);

	if (checkFrustum(pos, 1.0f)) {
		indirectDraws[idx].instanceCount = 1;
		indirectDraws[idx].firstIndex = instances[idx].firstIndex;
		indirectDraws[idx].indexCount = instances[idx].indexCount;

		InterlockedAdd(drawCount, 1);
	}
	else {
		indirectDraws[idx].instanceCount = 0;
	}
}