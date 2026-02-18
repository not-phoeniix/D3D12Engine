#pragma once

#include <DirectXMath.h>
#include "Material.h"

struct TransformBuffer {
    DirectX::XMFLOAT4X4 world;
    DirectX::XMFLOAT4X4 view;
    DirectX::XMFLOAT4X4 proj;
    DirectX::XMFLOAT4X4 wit;
};

struct MaterialBuffer {
    uint32_t texture_indices[MATERIAL_MAX_TEXTURES];
    uint32_t texture_index_count;
};
