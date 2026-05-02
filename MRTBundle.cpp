#include "MRTBundle.h"

#include "Graphics.h"
#include <cstring>

static void create_texture(
    uint32_t width,
    uint32_t height,
    DXGI_FORMAT format,
    const float* clear_color,
    Microsoft::WRL::ComPtr<ID3D12Resource>* out_texture
) {
    D3D12_RESOURCE_DESC desc = {};
    desc.Alignment = 0;
    desc.DepthOrArraySize = 1;
    desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
    desc.Format = format;
    desc.Width = width;
    desc.Height = height;
    desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    desc.MipLevels = 1;
    desc.SampleDesc.Count = 1;
    desc.SampleDesc.Quality = 0;

    D3D12_HEAP_PROPERTIES heap_props = {};
    heap_props.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    heap_props.CreationNodeMask = 1;
    heap_props.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    heap_props.Type = D3D12_HEAP_TYPE_DEFAULT;
    heap_props.VisibleNodeMask = 1;

    D3D12_CLEAR_VALUE clear = {};
    memcpy(clear.Color, clear_color, sizeof(float) * 4);
    clear.Format = format;

    Graphics::Device->CreateCommittedResource(
        &heap_props,
        D3D12_HEAP_FLAG_NONE,
        &desc,
        D3D12_RESOURCE_STATE_COMMON,
        &clear,
        IID_PPV_ARGS(out_texture->GetAddressOf())
    );
}

void mrt_bundle_create(
    uint32_t width,
    uint32_t height,
    const DXGI_FORMAT* formats,
    const float* clear_colors,
    uint32_t count,
    MRTBundle* out_bundle
) {
    out_bundle->images = new Microsoft::WRL::ComPtr<ID3D12Resource>[count];
    out_bundle->srv_descriptors = new DescriptorDesc[count];
    out_bundle->uav_descriptors = new DescriptorDesc[count];
    out_bundle->rtv_descriptors = new D3D12_CPU_DESCRIPTOR_HANDLE[count];
    out_bundle->formats = new DXGI_FORMAT[count];
    memcpy(out_bundle->formats, formats, sizeof(DXGI_FORMAT) * count);
    out_bundle->clear_colors = new float[count * 4];
    memcpy(out_bundle->clear_colors, clear_colors, sizeof(float) * 4 * count);
    out_bundle->count = count;

    D3D12_DESCRIPTOR_HEAP_DESC heap_desc = {};
    heap_desc.NumDescriptors = count;
    heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    Graphics::Device->CreateDescriptorHeap(&heap_desc, IID_PPV_ARGS(out_bundle->rtv_descriptor_heap.GetAddressOf()));

    D3D12_CPU_DESCRIPTOR_HANDLE rtv_cpu_start = out_bundle->rtv_descriptor_heap->GetCPUDescriptorHandleForHeapStart();
    uint32_t desc_size = Graphics::Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

    for (uint32_t i = 0; i < count; i++) {
        out_bundle->rtv_descriptors[i] = rtv_cpu_start;
        out_bundle->rtv_descriptors[i].ptr += desc_size * i;
    }

    mrt_bundle_resize(width, height, out_bundle);
}

void mrt_bundle_resize(
    uint32_t width,
    uint32_t height,
    MRTBundle* out_bundle
) {
    D3D12_RENDER_TARGET_VIEW_DESC rtv_desc = {};
    rtv_desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
    rtv_desc.Texture2D.MipSlice = 0;

    D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
    srv_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srv_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srv_desc.Texture2D.MipLevels = 1;
    srv_desc.Texture2D.MostDetailedMip = 0;

    for (uint32_t i = 0; i < out_bundle->count; i++) {
        create_texture(
            width,
            height,
            out_bundle->formats[i],
            &out_bundle->clear_colors[i * out_bundle->count],
            &out_bundle->images[i]
        );

        rtv_desc.Format = out_bundle->formats[i];
        Graphics::Device->CreateRenderTargetView(
            out_bundle->images[i].Get(),
            &rtv_desc,
            out_bundle->rtv_descriptors[i]
        );

        Graphics::ReserveDescriptorHeapSlot(
            &out_bundle->srv_descriptors[i].cpu_handle,
            &out_bundle->srv_descriptors[i].gpu_handle
        );

        srv_desc.Format = out_bundle->formats[i];
        Graphics::Device->CreateShaderResourceView(
            out_bundle->images[i].Get(),
            &srv_desc,
            out_bundle->srv_descriptors[i].cpu_handle
        );

        out_bundle->srv_descriptors[i].bindless_index = Graphics::get_descriptor_index(
            out_bundle->srv_descriptors[i].gpu_handle
        );
    }
}

void mrt_bundle_destroy(MRTBundle* bundle) {
    delete[] bundle->images;
    delete[] bundle->srv_descriptors;
    delete[] bundle->uav_descriptors;
    delete[] bundle->rtv_descriptors;
    delete[] bundle->formats;
    delete[] bundle->clear_colors;

    bundle->images = nullptr;
    bundle->srv_descriptors = nullptr;
    bundle->uav_descriptors = nullptr;
    bundle->formats = nullptr;
    bundle->clear_colors = nullptr;
    bundle->count = 0;
}
