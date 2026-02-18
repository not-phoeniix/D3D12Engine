#pragma once

#include <DirectXMath.h>
#include <d3d12.h>
#include <wrl/client.h>
#include <stdint.h>

constexpr uint32_t MATERIAL_MAX_TEXTURES = 32;

class Material {
   private:
    DirectX::XMFLOAT3 color_tint;
    DirectX::XMFLOAT2 uv_scale;
    DirectX::XMFLOAT2 uv_offset;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> pipeline_state;
    uint32_t texture_indices[MATERIAL_MAX_TEXTURES];
    uint32_t texture_index_count;

   public:
    Material(Microsoft::WRL::ComPtr<ID3D12PipelineState> pipeline_state);
    ~Material();
    Material(const Material& other);
    Material& operator=(const Material& other);

    bool AddTexture(uint32_t texture_index);
    void ClearTextures();

    DirectX::XMFLOAT3 get_color_tint() const { return color_tint; }
    DirectX::XMFLOAT2 get_uv_scale() const { return uv_scale; }
    DirectX::XMFLOAT2 get_uv_offset() const { return uv_offset; }
    Microsoft::WRL::ComPtr<ID3D12PipelineState> get_pipeline_state() const { return pipeline_state; }
    const uint32_t* get_texture_indices() const { return texture_indices; }
    uint32_t get_texture_index_count() const { return texture_index_count; }
    void set_color_tint(DirectX::XMFLOAT3 color_tint) { this->color_tint = color_tint; }
    void set_uv_scale(DirectX::XMFLOAT2 uv_scale) { this->uv_scale = uv_scale; }
    void set_uv_offset(DirectX::XMFLOAT2 uv_offset) { this->uv_offset = uv_offset; }
};
