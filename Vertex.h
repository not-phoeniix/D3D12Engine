#pragma once

#include <DirectXMath.h>
#include <vector>
#include <d3d12.h>

struct Vertex {
    DirectX::XMFLOAT3 Position;
    DirectX::XMFLOAT2 UV;
    DirectX::XMFLOAT3 Normal;
    DirectX::XMFLOAT3 Tangent;
};

std::vector<D3D12_INPUT_ELEMENT_DESC> vertex_get_input_elements();
