struct VertexShaderInput { 
	float3 position : POSITION;
	float2 uv : TEXCOORD;
	float3 normal : NORMAL;
	float3 tangent : TANGENT;
};

struct VertexToPixel {
	float4 position : SV_POSITION;
};

cbuffer MatrixData : register(b0) {
	float4x4 world;
	float4x4 view;
	float4x4 proj;
}

VertexToPixel main(VertexShaderInput input) {
	VertexToPixel output;

	float4x4 wvp = mul(proj, mul(view, world));

	output.position = mul(wvp, float4(input.position, 1.0f));

	return output;
}