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

uint get_texture_index(uint mat_index) {
	uint arr_index = mat_index / 4;
	uint vec_index = mat_index % 4;
	return texture_indices[arr_index][vec_index];
}

float4 main(PostProcessIn input) : SV_TARGET {
	Texture2D albedo_tex = ResourceDescriptorHeap[albedo_rt_id];
	Texture2D material_tex = ResourceDescriptorHeap[material_rt_id];
	Texture2D normals_tex = ResourceDescriptorHeap[normals_rt_id];
	Texture2D world_pos_depth_tex = ResourceDescriptorHeap[world_pos_depth_rt_id];
	TextureCube sky_cubemap = ResourceDescriptorHeap[skybox_cubemap_id];

    float3 albedo = albedo_tex.Sample(BasicSampler, input.uv).xyz;
    float2 material = material_tex.Sample(BasicSampler, input.uv).xy;
    float3 normal = normals_tex.Sample(BasicSampler, input.uv).xyz * 2.0f - float3(1.0f, 1.0f, 1.0f);
    float4 world_pos_depth_sample = world_pos_depth_tex.Sample(BasicSampler, input.uv);
    float roughness = material.x;
    float metalness = material.y;
    float depth = world_pos_depth_sample.w;

    PSInput light_input;
    // unpack what we packed before
    light_input.world_pos = world_pos_depth_sample.xyz * 1000.0f;
    light_input.normal = normal;

	float3 specular_color = lerp(F0_NON_METAL.rrr, albedo, metalness);
	float3 total_light = float3(0.0, 0.0, 0.0);

	uint light_iterations = min(light_count, MAX_LIGHTS);
	for (uint i = 0; i < light_iterations; i++) {
		Light light = lights[i];
		light.direction = normalize(light.direction);

		switch (light.type) {
			case LIGHT_TYPE_DIRECTIONAL:
				total_light += LightDirectionalPBR(light, light_input, camera_world_pos, roughness, specular_color, metalness, albedo);
				break;
			case LIGHT_TYPE_POINT:
				total_light += LightPointPBR(light, light_input, camera_world_pos, roughness, specular_color, metalness, albedo);
				break;
			case LIGHT_TYPE_SPOT:
				total_light += LightSpotPBR(light, light_input, camera_world_pos, roughness, specular_color, metalness, albedo);
				break;
		}
	}

	// add skybox reflection
	float3 to_frag = normalize(light_input.world_pos - camera_world_pos);
	float3 refl_vec = reflect(to_frag, normal);
	float3 sky_refl = sky_cubemap.Sample(BasicSampler, refl_vec).rgb;

	float3 sky_surface_blend = lerp(
		total_light, 
		sky_refl, 
		FresnelApprox(normal, -to_frag)
	);

	// gamma correction
	return float4(pow(abs(sky_surface_blend), 1.0 / gamma), 1);
}