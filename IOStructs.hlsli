#ifndef IOSTRUCTS_H
#define IOSTRUCTS_H

struct VertexShaderInput { 
	float3 position : POSITION;
	float2 uv : TEXCOORD;
	float3 normal : NORMAL;
	float3 tangent : TANGENT;
};

struct VertexToPixel {
	float4 position : SV_POSITION;
	float2 uv : TEXCOORD0;
	float3 normal : NORMAL;
	float3 tangent : TANGENT;
	float3 worldPos : TEXCOORD1;
};

#endif
