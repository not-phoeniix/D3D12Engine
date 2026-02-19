#include "Material.h"

Material::Material(Microsoft::WRL::ComPtr<ID3D12PipelineState> pipeline_state)
  : color_tint(1.0f, 1.0f, 1.0f),
    uv_scale(1.0f, 1.0f),
    uv_offset(0.0f, 0.0f),
    pipeline_state(pipeline_state),
    texture_index_count(0) { }

Material::~Material() { }

Material::Material(const Material& other)
  : color_tint(other.color_tint),
    uv_scale(other.uv_scale),
    uv_offset(other.uv_offset),
    pipeline_state(other.pipeline_state),
    texture_index_count(other.texture_index_count) {
    memcpy(texture_indices, other.texture_indices, sizeof(texture_indices));
}

Material& Material::operator=(const Material& other) {
    color_tint = other.color_tint;
    uv_scale = other.uv_scale;
    uv_offset = other.uv_offset;
    pipeline_state = other.pipeline_state;
    memcpy(texture_indices, other.texture_indices, sizeof(texture_indices));
    texture_index_count = other.texture_index_count;
    return *this;
}

bool Material::AddTexture(uint32_t texture_index) {
    if (texture_index_count >= MATERIAL_MAX_TEXTURES) {
        return false;
    }

    texture_indices[texture_index_count++] = texture_index;
    return true;
}

void Material::ClearTextures() {
    memset(texture_indices, 0, sizeof(texture_indices));
    texture_index_count = 0;
}
