struct VertexToPixel {
	float4 screenPosition	: SV_POSITION;
};

float4 main(VertexToPixel input) : SV_TARGET {
	return float4(1.0, 1.0, 1.0, 1.0);
}
