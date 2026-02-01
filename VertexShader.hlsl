struct VertexShaderInput { 
	float3 localPosition	: POSITION;
	float4 color			: COLOR;
};

struct VertexToPixel {
	float4 screenPosition	: SV_POSITION;
	float4 color			: COLOR; 
};

VertexToPixel main(VertexShaderInput input) {
	VertexToPixel output;

	output.screenPosition = float4(input.localPosition, 1.0f);
	output.color = input.color;

	return output;
}