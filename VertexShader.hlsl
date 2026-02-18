#include "IOStructs.hlsli"

cbuffer MatrixData : register(b0) {
	float4x4 world;
	float4x4 view;
	float4x4 proj;
	float4x4 wit;
}

VertexToPixel main(VertexShaderInput input) {
	VertexToPixel output;

	float4x4 wvp = mul(proj, mul(view, world));

	output.position = mul(wvp, float4(input.position, 1.0f));
	output.worldPos = mul(world, float4(input.position, 1.0f)).xyz;

	output.uv = input.uv;

	output.normal = normalize(mul((float3x3)wit, input.normal));
	output.tangent = normalize(mul((float3x3)world, input.tangent));

	return output;
}