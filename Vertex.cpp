#include "Vertex.h"

std::vector<D3D12_INPUT_ELEMENT_DESC> vertex_get_input_elements() {
    std::vector<D3D12_INPUT_ELEMENT_DESC> elements;
    elements.resize(4);

    elements[0].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
    elements[0].Format = DXGI_FORMAT_R32G32B32_FLOAT;
    elements[0].SemanticName = "POSITION";
    elements[0].SemanticIndex = 0;

    elements[1].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
    elements[1].Format = DXGI_FORMAT_R32G32_FLOAT;
    elements[1].SemanticName = "TEXCOORD";
    elements[1].SemanticIndex = 0;

    elements[2].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
    elements[2].Format = DXGI_FORMAT_R32G32B32_FLOAT;
    elements[2].SemanticName = "NORMAL";
    elements[2].SemanticIndex = 0;

    elements[3].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
    elements[3].Format = DXGI_FORMAT_R32G32B32_FLOAT;
    elements[3].SemanticName = "TANGENT";
    elements[3].SemanticIndex = 0;

    return elements;
}
