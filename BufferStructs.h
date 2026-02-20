#pragma once

#include <DirectXMath.h>
#include "Material.h"
#include "Light.h"

#define MATERIAL_BUFFER_PACKED_VECTOR_COUNT (MATERIAL_MAX_TEXTURES + 3) / 4
#define MAX_LIGHTS 128

struct TransformBuffer {
    DirectX::XMFLOAT4X4 world;
    DirectX::XMFLOAT4X4 view;
    DirectX::XMFLOAT4X4 proj;
    DirectX::XMFLOAT4X4 wit;
};

struct SceneDataBuffer {
    DirectX::XMFLOAT3 camera_world_pos;
    float gamma;
    Light lights[MAX_LIGHTS];
    uint32_t light_count;
};

struct MaterialBuffer {
    DirectX::XMFLOAT2 uv_scale;
    DirectX::XMFLOAT2 uv_offset;
    DirectX::XMFLOAT3 color_tint;
    uint32_t texture_index_count;
    DirectX::XMUINT4 packed_texture_indices[MATERIAL_BUFFER_PACKED_VECTOR_COUNT];
};
