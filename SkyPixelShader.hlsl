#include "IOStructs.hlsli"

SamplerState BasicSampler : register(s0);

cbuffer SkyInfoConstants : register(b0) {
    uint cubemap_id;
}

float4 main(SkyPSIn input) : SV_TARGET {
    TextureCube tex = ResourceDescriptorHeap[cubemap_id];
	return tex.Sample(BasicSampler, input.sample_dir);
}
