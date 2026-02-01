#pragma once

#include <d3d12.h>
#include <wrl/client.h>

class Game {
   public:
    // Basic OOP setup
    Game();
    ~Game();
    Game(const Game&) = delete;            // Remove copy constructor
    Game& operator=(const Game&) = delete; // Remove copy-assignment operator

    // Primary functions
    void Update(float deltaTime, float totalTime);
    void Draw(float deltaTime, float totalTime);
    void OnResize();

   private:
    void CreateRootSigAndPipelineState();
    void CreateGeometry();
    void ClearPrevFrame();
    void Present();

    Microsoft::WRL::ComPtr<ID3D12RootSignature> root_signature;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> pipeline_state;

    Microsoft::WRL::ComPtr<ID3D12Resource> vertex_buffer;
    D3D12_VERTEX_BUFFER_VIEW vertex_buffer_view = {};
    Microsoft::WRL::ComPtr<ID3D12Resource> index_buffer;
    D3D12_INDEX_BUFFER_VIEW index_buffer_view = {};

    D3D12_VIEWPORT viewport = {};
    D3D12_RECT scissor_rect = {};
};

