#pragma once

#include <d3d12.h>
#include <wrl/client.h>
#include <cstdint>

// lol, descriptor description
struct DescriptorDesc {
    D3D12_CPU_DESCRIPTOR_HANDLE cpu_handle;
    D3D12_GPU_DESCRIPTOR_HANDLE gpu_handle;
    uint32_t bindless_index = -1;
};

// a bundle of data, pointers, etc that represent all resources
//   needed for rendering to multiple render targets for a single frame
struct MRTBundle {
    Microsoft::WRL::ComPtr<ID3D12Resource>* images;
    DescriptorDesc* srv_descriptors;
    DescriptorDesc* uav_descriptors;
    // we can just use the raw CPU handle cuz we'll feed this as an 
    //   array into the draw call directly later
    D3D12_CPU_DESCRIPTOR_HANDLE* rtv_descriptors;
    DXGI_FORMAT* formats;
    float* clear_colors;
    // unique heap for all our descriptors individually bundled w everything else
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> rtv_descriptor_heap;
    uint32_t count;
};

void mrt_bundle_create(
    uint32_t width,
    uint32_t height,
    const DXGI_FORMAT* formats,
    const float* clear_colors,
    uint32_t count,
    MRTBundle* out_bundle
);
void mrt_bundle_resize(
    uint32_t width,
    uint32_t height,
    MRTBundle* out_bundle
);
void mrt_bundle_destroy(MRTBundle* bundle);
