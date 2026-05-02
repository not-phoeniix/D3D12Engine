#ifndef IOSTRUCTS_H
#define IOSTRUCTS_H

struct VSInput { 
	float3 position : POSITION;
	float2 uv : TEXCOORD;
	float3 normal : NORMAL;
	float3 tangent : TANGENT;
};

struct PSInput {
	float4 position : SV_POSITION;
	float2 uv : TEXCOORD0;
	float3 normal : NORMAL;
	float3 tangent : TANGENT;
	float3 world_pos : TEXCOORD1;
};

struct PostProcessIn {
	float4 position: SV_POSITION;
	float2 uv : TEXCOORD0;
};

struct SkyPSIn {
	float4 position : SV_POSITION;
	float3 sample_dir : DIRECTION;
};

struct MRTOut {
	float4 albedo: SV_TARGET0;
	float4 normals: SV_TARGET1;
	float4 material: SV_TARGET2;
	float4 world_pos_depth: SV_TARGET3;
};

#endif
