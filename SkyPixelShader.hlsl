#include "IOStructs.hlsli"

SamplerState BasicSampler : register(s0);

cbuffer SkyInfoConstants : register(b0) {
    uint cubemap_id;
}

MRTOut main(SkyPSIn input) : SV_TARGET {
    TextureCube tex = ResourceDescriptorHeap[cubemap_id];
	float3 color = tex.Sample(BasicSampler, input.sample_dir).rgb;

    MRTOut output;
    // alpha channel stands for light mask (skybox should not be lit)
    output.albedo = float4(color, 0.0f);

	return output;
}
