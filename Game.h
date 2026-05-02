#pragma once

#include <d3d12.h>
#include <wrl/client.h>
#include "Camera.h"
#include "GameEntity.h"
#include "Light.h"
#include "Graphics.h"
#include "MRTBundle.h"

constexpr float GAME_GAMMA = 1.4f;

constexpr uint32_t ALBEDO_RT_IDX = 0;
constexpr uint32_t NORMALS_RT_IDX = 1;
constexpr uint32_t MATERIAL_RT_IDX = 2;
constexpr uint32_t DEPTH_RT_IDX = 3;

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
    void CreateMainPipelineStuff();
    void InitSky();
    void SceneInit();
    void ClearPrevFrame();
    void Present();

    void RandomizeLights();

    // mrt stuff
    MRTBundle mrt_bundles[Graphics::NUM_BACK_BUFFERS];

    Microsoft::WRL::ComPtr<ID3D12RootSignature> root_signature;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> mrt_pipeline_state;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> fullscreen_pipeline_state;
    Microsoft::WRL::ComPtr<ID3D12RootSignature> sky_root_signature;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> sky_pipeline_state;
    std::shared_ptr<Mesh> cube_mesh;
    uint32_t sky_cubemap_id;

    D3D12_VIEWPORT viewport = {};
    D3D12_RECT scissor_rect = {};

    std::unique_ptr<Camera> camera;
    std::vector<GameEntity> entities;
    std::vector<Light> lights;
};

