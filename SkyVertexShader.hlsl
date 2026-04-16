#include "IOStructs.hlsli"

cbuffer SkyMatrixBuffer : register(b0) {
	float4x4 view;
	float4x4 projection;
}

SkyPSIn main(VSInput input) {
	SkyPSIn output;

	matrix viewNoTranslate = view;
	viewNoTranslate._14 = 0;
	viewNoTranslate._24 = 0;
	viewNoTranslate._34 = 0;

	matrix viewProj = mul(projection, viewNoTranslate);

	output.position = mul(viewProj, float4(input.position, 1.0f));
	output.position.z = output.position.w;
	output.sample_dir = input.position;

	return output;
}
