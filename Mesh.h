#pragma once

#include <stdint.h>
#include <d3d12.h>
#include <wrl/client.h>
#include <memory>
#include "Vertex.h"

class Mesh {
   private:
    uint32_t num_vertices;
    uint32_t num_indices;
    Microsoft::WRL::ComPtr<ID3D12Resource> vertex_buffer;
    D3D12_VERTEX_BUFFER_VIEW vertex_buffer_view;
    Microsoft::WRL::ComPtr<ID3D12Resource> index_buffer;
    D3D12_INDEX_BUFFER_VIEW index_buffer_view;

   public:
    Mesh(const Vertex* vertices, uint32_t vertex_count, const uint32_t* indices, uint32_t index_count);
    ~Mesh();

    D3D12_VERTEX_BUFFER_VIEW get_vb_view() const { return vertex_buffer_view; }
    D3D12_INDEX_BUFFER_VIEW get_ib_view() const { return index_buffer_view; }
    uint32_t get_vertex_count() const { return num_vertices; }
    uint32_t get_index_count() const { return num_indices; }

    static std::shared_ptr<Mesh> Load(const char* path);
};
