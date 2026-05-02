#include "IOStructs.hlsli"
#include "Lighting.hlsli"

//! make sure this matches the constexpr in "Material.h" !!!!
#define MATERIAL_MAX_TEXTURES 32
#define PACKED_VECTOR_COUNT (MATERIAL_MAX_TEXTURES + 3) / 4

SamplerState BasicSampler : register(s0);

cbuffer SceneData : register(b0) {
	float3 camera_world_pos;
	float gamma;
	Light lights[MAX_LIGHTS];
	uint light_count;
	uint skybox_cubemap_id;
    uint albedo_rt_id;
    uint normals_rt_id;
    uint material_rt_id;
    uint world_pos_depth_rt_id;
};

cbuffer MaterialData : register(b1) {
	float2 uv_scale;
	float2 uv_offset;
	float3 color_tint;
	uint texture_count;
	uint4 texture_indices[PACKED_VECTOR_COUNT];
}

struct MRTOut {
	float4 albedo: SV_TARGET0;
	float4 normals: SV_TARGET1;
	float4 material: SV_TARGET2;
	float4 world_pos_depth: SV_TARGET3;
};

uint get_texture_index(uint mat_index) {
	uint arr_index = mat_index / 4;
	uint vec_index = mat_index % 4;
	return texture_indices[arr_index][vec_index];
}

float3 get_normal(Texture2D normal_tex, SamplerState s, PSInput input) {
	// TBN matrix
	float3 N = normalize(input.normal);
	float3 T = normalize(input.tangent - N * dot(input.tangent, N));
	float3 B = cross(T, N);
	float3x3 TBN = float3x3(T, B, N);

	// sample, unpack, transform, and save normal map values
	float3 unpacked_normal = normal_tex.Sample(s, input.uv).xyz * 2.0 - 1.0;
	return normalize(mul(unpacked_normal, TBN));
}

MRTOut main(PSInput input) {
	Texture2D albedo = ResourceDescriptorHeap[get_texture_index(0)];
	Texture2D metalness_map = ResourceDescriptorHeap[get_texture_index(1)];
	Texture2D normal_map = ResourceDescriptorHeap[get_texture_index(2)];
	Texture2D roughness_map = ResourceDescriptorHeap[get_texture_index(3)];		

	// apply UV modifications
	input.uv *= uv_scale;
	input.uv += uv_offset;

	// clean up tangents & normals (sometimes they aren't normalized)
	input.normal = normalize(input.normal);
	input.tangent = normalize(input.tangent);

	// set up lighting parameters before light calculations
	input.normal = get_normal(normal_map, BasicSampler, input);
    float3 surface_color = albedo.Sample(BasicSampler, input.uv).rgb * color_tint;
	float roughness = roughness_map.Sample(BasicSampler, input.uv).r;
	float metalness = metalness_map.Sample(BasicSampler, input.uv).r;

	MRTOut output;
	output.albedo = float4(surface_color, 1);
	output.normals = float4(input.normal * 0.5f + 0.5f, 1.0f);
	output.material = float4(roughness, metalness, 0.0f, 1.0f);
	// im just gonna divide by a thousand to adhere to texture clamp restraints
	output.world_pos_depth = float4(input.world_pos / 1000.0f, input.position.z);

	return output;

}
