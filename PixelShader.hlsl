struct VertexToPixel {
	float4 position : SV_POSITION;
	float2 uv : TEXCOORD;
};

float4 main(VertexToPixel input) : SV_TARGET {
	// return float4(1.0, 1.0, 1.0, 1.0);
	return float4(input.uv, 0.0, 1.0);
}
