#include "IOStructs.hlsli"

//! make sure this matches the constexpr in "Material.h" !!!!
#define MATERIAL_MAX_TEXTURES 32
#define PACKED_VECTOR_COUNT (MATERIAL_MAX_TEXTURES + 3) / 4
#define TEXTURE_SPACE space1

SamplerState BasicSampler : register(s0);

cbuffer MaterialData : register(b0) {
	uint4 texture_indices[PACKED_VECTOR_COUNT];
	uint texture_count;
}

Texture2D TextureHeap[] : register(t0, TEXTURE_SPACE);

uint get_texture_index(uint mat_index) {
	uint arr_index = mat_index / 4;
	uint vec_index = mat_index % 4;
	return texture_indices[arr_index][vec_index];
}

float4 main(VertexToPixel input) : SV_TARGET {
	Texture2D albedo = TextureHeap[get_texture_index(0)];
	Texture2D metal = TextureHeap[get_texture_index(1)];
	Texture2D normals = TextureHeap[get_texture_index(2)];
	Texture2D roughness = TextureHeap[get_texture_index(3)];

	float4 color = normals.Sample(BasicSampler, input.uv);

	return color;
}
