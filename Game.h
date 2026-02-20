#pragma once

#include <d3d12.h>
#include <wrl/client.h>
#include "Camera.h"
#include "GameEntity.h"
#include "Light.h"

constexpr float GAME_GAMMA = 1.4f;

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
    void SceneInit();
    void ClearPrevFrame();
    void Present();
    
    void RandomizeLights();

    Microsoft::WRL::ComPtr<ID3D12RootSignature> root_signature;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> pipeline_state;

    D3D12_VIEWPORT viewport = {};
    D3D12_RECT scissor_rect = {};

    std::unique_ptr<Camera> camera;
    std::vector<GameEntity> entities;
    std::vector<Light> lights;
};

