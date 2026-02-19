#include "IOStructs.hlsli"

//! make sure this matches the constexpr in "Material.h" !!!!
#define MATERIAL_MAX_TEXTURES 32
#define TEXTURE_SPACE space1

SamplerState BasicSampler : register(s0);

cbuffer MaterialData : register(b0) {
	uint texture_indices[MATERIAL_MAX_TEXTURES];
	uint texture_count;
}

Texture2D TextureHeap[] : register(t0, TEXTURE_SPACE);

float4 main(VertexToPixel input) : SV_TARGET {
	Texture2D albedo = TextureHeap[texture_indices[0]];
	// Texture2D albedo = ResourceDescriptorHeap[texture_indices[0]];
	float4 color = albedo.Sample(BasicSampler, input.uv);

	return color;
	// return float4(1.0f, 1.0f, 1.0f, 1.0f);
}
