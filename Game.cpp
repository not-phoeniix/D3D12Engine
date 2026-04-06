#include "Game.h"
#include "Graphics.h"
#include "Vertex.h"
#include "Input.h"
#include "PathHelpers.h"
#include "Window.h"
#include <vector>
#include "BufferStructs.h"
#include <DirectXMath.h>
#include <cstdlib>
#include "RayTracing.h"

// Needed for a helper function to load pre-compiled shader files
#pragma comment(lib, "d3dcompiler.lib")
#include <d3dcompiler.h>

// For the DirectX Math library
using namespace DirectX;

static float randf_range(float min, float max) {
    return min + ((max - min) * ((rand() / (float)RAND_MAX)));
}

// --------------------------------------------------------
// The constructor is called after the window and graphics API
// are initialized but before the game loop begins
// --------------------------------------------------------
Game::Game() {
    RayTracing::Initialize(
        Window::Width(),
        Window::Height(),
        FixPath(L"RayTracing.cso")
    );

    SceneInit();
}

// --------------------------------------------------------
// Clean up memory or objects created by this class
//
// Note: Using smart pointers means there probably won't
//       be much to manually clean up here!
// --------------------------------------------------------
Game::~Game() {
    Graphics::WaitForGPU();
}

// --------------------------------------------------------
// Initializes scene objects like cameras and entities
// --------------------------------------------------------
void Game::SceneInit() {
    camera = std::make_shared<Camera>(
        DirectX::XMFLOAT3(0.0f, 0.0f, -10.0f),
        5.0f,
        0.005f,
        XM_PIDIV2,
        Window::AspectRatio(),
        0.01f,
        100.0f
    );

    RandomizeLights();

    std::shared_ptr<Material> mat_bronze = std::make_shared<Material>(pipeline_state);
    {
        mat_bronze->AddTexture(Graphics::LoadTexture(FixPath(L"../../Assets/Textures/bronze_albedo.png").c_str()));
        mat_bronze->AddTexture(Graphics::LoadTexture(FixPath(L"../../Assets/Textures/bronze_metal.png").c_str()));
        mat_bronze->AddTexture(Graphics::LoadTexture(FixPath(L"../../Assets/Textures/bronze_normals.png").c_str()));
        mat_bronze->AddTexture(Graphics::LoadTexture(FixPath(L"../../Assets/Textures/bronze_roughness.png").c_str()));
    }

    std::shared_ptr<Material> mat_cobblestone = std::make_shared<Material>(pipeline_state);
    {
        mat_cobblestone->AddTexture(Graphics::LoadTexture(FixPath(L"../../Assets/Textures/cobblestone_albedo.png").c_str()));
        mat_cobblestone->AddTexture(Graphics::LoadTexture(FixPath(L"../../Assets/Textures/cobblestone_metal.png").c_str()));
        mat_cobblestone->AddTexture(Graphics::LoadTexture(FixPath(L"../../Assets/Textures/cobblestone_normals.png").c_str()));
        mat_cobblestone->AddTexture(Graphics::LoadTexture(FixPath(L"../../Assets/Textures/cobblestone_roughness.png").c_str()));
        mat_cobblestone->set_uv_scale({0.25f, 0.25f});
    }

    std::shared_ptr<Material> mat_floor = std::make_shared<Material>(pipeline_state);
    {
        mat_floor->AddTexture(Graphics::LoadTexture(FixPath(L"../../Assets/Textures/floor_albedo.png").c_str()));
        mat_floor->AddTexture(Graphics::LoadTexture(FixPath(L"../../Assets/Textures/floor_metal.png").c_str()));
        mat_floor->AddTexture(Graphics::LoadTexture(FixPath(L"../../Assets/Textures/floor_normals.png").c_str()));
        mat_floor->AddTexture(Graphics::LoadTexture(FixPath(L"../../Assets/Textures/floor_roughness.png").c_str()));
        mat_floor->set_uv_scale({2.0f, 2.0f});
    }

    auto sphere_mesh = Mesh::Load(FixPath("../../Assets/Meshes/cube.obj").c_str());

    entities.resize(20);
    for (size_t i = 0; i < entities.size(); i++) {
        auto material = std::make_shared<Material>(pipeline_state);
        material->set_color_tint({
            randf_range(0.0f, 1.0f),
            randf_range(0.0f, 1.0f),
            randf_range(0.0f, 1.0f),
        });

        entities[i] = std::make_shared<GameEntity>(
            sphere_mesh,
            material,
            Transform({randf_range(-10.0f, 10.0f), 0.0f, randf_range(-10.0f, 10.0f)})
        );
    }
    entities[0]->get_transform().SetScale(1000.0f);
    entities[0]->get_transform().SetPosition({0, -1000.0f, 0});
    entities[0]->get_material()->set_color_tint({0.01f, 0.01f, 0.01f});

    RayTracing::CreateEntityDataBuffer(entities);
    RayTracing::CreateTopLevelAccelerationStructureForScene(entities);

    Graphics::CloseAndExecuteCommandList();
    Graphics::WaitForGPU();
    Graphics::ResetAllocatorAndCommandList(Graphics::get_swap_chain_index());
}

// --------------------------------------------------------
// Handle resizing to match the new window size
//  - Eventually, we'll want to update our 3D camera
// --------------------------------------------------------
void Game::OnResize() {
    RayTracing::ResizeOutputUAV(Window::Width(), Window::Height());

    viewport = {};
    viewport.TopLeftX = 0.0f;
    viewport.TopLeftY = 0.0f;
    viewport.Width = (float)Window::Width();
    viewport.Height = (float)Window::Height();
    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;

    scissor_rect = {};
    scissor_rect.left = 0;
    scissor_rect.top = 0;
    scissor_rect.right = Window::Width();
    scissor_rect.bottom = Window::Height();

    camera->UpdateProjectionMatrix(Window::AspectRatio());
}

// --------------------------------------------------------
// Update your game here - user input, move objects, AI, etc.
// --------------------------------------------------------
void Game::Update(float deltaTime, float totalTime) {
    if (Input::KeyDown(VK_ESCAPE)) {
        Window::Quit();
    }

    if (Input::KeyPress(VK_SPACE)) {
        RandomizeLights();
    }

    camera->Update(deltaTime);

    for (auto& entity : entities) {
        // entity->get_transform().Rotate(0, deltaTime, 0);
    }
}

// --------------------------------------------------------
// Clear the screen, redraw everything, present to the user
// --------------------------------------------------------
void Game::Draw(float deltaTime, float totalTime) {
    // we dont actually need to clear since the frame is automatically overwritten every frame
    // ClearPrevFrame();

    // our actual rendering things happen between clearing and presenting !!!!!

    auto current_back_buffer = Graphics::BackBuffers[Graphics::get_swap_chain_index()];
    auto command_list = Graphics::CommandList;

    RayTracing::CreateTopLevelAccelerationStructureForScene(entities);
    RayTracing::Raytrace(camera, current_back_buffer);

    // command_list->SetDescriptorHeaps(1, Graphics::CBVSRVDescriptorHeap.GetAddressOf());
    // command_list->SetGraphicsRootSignature(root_signature.Get());

    // set parameters & objects & references & etc for upcoming draw
    // command_list->OMSetRenderTargets(
    //     1,
    //     &Graphics::RTVHandles[Graphics::get_swap_chain_index()],
    //     true,
    //     &Graphics::DSVHandle
    // );
    // command_list->RSSetViewports(1, &viewport);
    // command_list->RSSetScissorRects(1, &scissor_rect);
    // command_list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    // copy scene data - ONCE PER FRAME
    // {
    //     SceneDataBuffer scene_data = {};
    //     scene_data.camera_world_pos = camera->GetTransform().GetPosition();
    //     scene_data.gamma = GAME_GAMMA;
    //     scene_data.light_count = static_cast<uint32_t>(lights.size());
    //     memcpy(
    //         scene_data.lights,
    //         lights.data(),
    //         sizeof(Light) * lights.size()
    //     );

    //     D3D12_GPU_DESCRIPTOR_HANDLE handle = Graphics::CBHeapFillNext(&scene_data, sizeof(scene_data));
    //     command_list->SetGraphicsRootDescriptorTable(1, handle);
    // }

    // for (auto& entity : entities) {
    //     std::shared_ptr<Mesh> mesh = entity->get_mesh();
    //     std::shared_ptr<Material> material = entity->get_material();

    //     // transform buffer
    //     {
    //         TransformBuffer data = {};
    //         data.world = entity->get_transform().GetWorldMatrix();
    //         data.view = camera->GetView();
    //         data.proj = camera->GetProjection();
    //         data.wit = entity->get_transform().GetWorldInverseTransposeMatrix();

    //         D3D12_GPU_DESCRIPTOR_HANDLE handle = Graphics::CBHeapFillNext(&data, sizeof(data));
    //         command_list->SetGraphicsRootDescriptorTable(0, handle);
    //     }

    //     // material buffer
    //     {
    //         MaterialBuffer data = {};
    //         uint32_t texture_count = material->get_texture_index_count();
    //         memcpy(
    //             data.packed_texture_indices,
    //             material->get_texture_indices(),
    //             sizeof(uint32_t) * texture_count
    //         );
    //         data.texture_index_count = texture_count;
    //         data.uv_offset = material->get_uv_offset();
    //         data.uv_scale = material->get_uv_scale();
    //         data.color_tint = material->get_color_tint();

    //         D3D12_GPU_DESCRIPTOR_HANDLE handle = Graphics::CBHeapFillNext(&data, sizeof(data));
    //         command_list->SetGraphicsRootDescriptorTable(2, handle);
    //     }

    //     command_list->SetPipelineState(material->get_pipeline_state().Get());

    //     D3D12_VERTEX_BUFFER_VIEW vb_view = mesh->get_vb_view();
    //     command_list->IASetVertexBuffers(0, 1, &vb_view);
    //     D3D12_INDEX_BUFFER_VIEW ib_view = mesh->get_ib_view();
    //     command_list->IASetIndexBuffer(&ib_view);

    //     command_list->DrawIndexedInstanced(mesh->get_index_count(), 1, 0, 0, 0);
    // }

    Present();
}

void Game::ClearPrevFrame() {
    auto current_back_buffer = Graphics::BackBuffers[Graphics::get_swap_chain_index()];
    auto command_list = Graphics::CommandList;

    // transition layout from present to render target layout
    D3D12_RESOURCE_BARRIER rb = {};
    rb.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    rb.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    rb.Transition.pResource = current_back_buffer.Get();
    rb.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
    rb.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
    rb.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    command_list->ResourceBarrier(1, &rb);

    // clear main render target
    float color[] = {0.0f, 0.0f, 0.0f, 1.0f};
    command_list->ClearRenderTargetView(
        Graphics::RTVHandles[Graphics::get_swap_chain_index()],
        color,
        0,      // no scissor rects
        nullptr // no scissor rects
    );

    // clear depth buffer
    command_list->ClearDepthStencilView(
        Graphics::DSVHandle,
        D3D12_CLEAR_FLAG_DEPTH,
        1.0f,   // clear depth @ 1.0f (max)
        0,      // don't care ab stencil clear value
        0,      // no scissor rects
        nullptr // no scissor rects
    );
}

void Game::Present() {
    auto current_back_buffer = Graphics::BackBuffers[Graphics::get_swap_chain_index()];
    auto command_list = Graphics::CommandList;

    // IMPORTANT!!!!! actually execute our list <3
    Graphics::CloseAndExecuteCommandList();

    // ACTUAL PRESENT :D
    bool vsync = Graphics::get_vsync_state();
    Graphics::SwapChain->Present(
        vsync ? 1 : 0,
        vsync ? 0 : DXGI_PRESENT_ALLOW_TEARING
    );

    // finalize things and prepare for next frame
    Graphics::AdvanceSwapChainIndex();
    Graphics::ResetAllocatorAndCommandList(Graphics::get_swap_chain_index());
}

void Game::RandomizeLights() {
    // randomize lights
    lights.resize(10);
    for (size_t i = 0; i < lights.size(); i++) {
        // rand type either 0, 1, 2
        lights[i].type = static_cast<uint32_t>(randf_range(0.0f, 2.999f));
        lights[i].range = randf_range(10.0f, 100.0f);
        lights[i].position = {
            randf_range(-50.0f, 50.0f),
            randf_range(-50.0f, 50.0f),
            randf_range(-50.0f, 50.0f)
        };
        // make all lights face towards the center
        DirectX::XMStoreFloat3(
            &lights[i].direction,
            DirectX::XMLoadFloat3(&lights[i].position) * -1.0f
        );
        lights[i].color = {
            randf_range(0.0f, 1.0f),
            randf_range(0.0f, 1.0f),
            randf_range(0.0f, 1.0f)
        };
        lights[i].spot_inner_angle = randf_range(0.0f, 2.0f);
        lights[i].spot_outer_angle = randf_range(0.0f, 2.0f);
        lights[i].intensity = randf_range(0.1f, 2.0f);
    }
}

