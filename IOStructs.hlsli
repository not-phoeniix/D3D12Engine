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

#endif
