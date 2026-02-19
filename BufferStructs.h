#pragma once

#include <DirectXMath.h>
#include "Material.h"

#define MATERIAL_BUFFER_PACKED_VECTOR_COUNT (MATERIAL_MAX_TEXTURES + 3) / 4

struct TransformBuffer {
    DirectX::XMFLOAT4X4 world;
    DirectX::XMFLOAT4X4 view;
    DirectX::XMFLOAT4X4 proj;
    DirectX::XMFLOAT4X4 wit;
};

struct MaterialBuffer {
    DirectX::XMUINT4 packed_texture_indices[MATERIAL_BUFFER_PACKED_VECTOR_COUNT];
    uint32_t texture_index_count;
};
