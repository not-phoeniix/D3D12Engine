#include "IOStructs.hlsli"

//! make sure this matches the constexpr in "Material.h" !!!!
#define MATERIAL_MAX_TEXTURES 32

cbuffer MaterialData : register(b0) {
	uint texture_indices[MATERIAL_MAX_TEXTURES];
	uint texture_count;
}

SamplerState Sampler : register(s0);

float4 main(VertexToPixel input) : SV_TARGET {
	Texture2D albedo = ResourceDescriptorHeap[texture_indices[0]];
	float4 color = albedo.Sample(Sampler, input.uv);

	return color;
}
