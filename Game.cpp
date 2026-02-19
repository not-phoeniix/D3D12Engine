#include "Game.h"
#include "Graphics.h"
#include "Vertex.h"
#include "Input.h"
#include "PathHelpers.h"
#include "Window.h"
#include <vector>
#include "BufferStructs.h"
#include <DirectXMath.h>

// Needed for a helper function to load pre-compiled shader files
#pragma comment(lib, "d3dcompiler.lib")
#include <d3dcompiler.h>

// For the DirectX Math library
using namespace DirectX;

// --------------------------------------------------------
// The constructor is called after the window and graphics API
// are initialized but before the game loop begins
// --------------------------------------------------------
Game::Game() {
    CreateRootSigAndPipelineState();
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

void Game::CreateRootSigAndPipelineState() {
    // load shader bytecode !!!
    Microsoft::WRL::ComPtr<ID3DBlob> vertex_shader_bytecode;
    Microsoft::WRL::ComPtr<ID3DBlob> pixel_shader_bytecode;
    {
        D3DReadFileToBlob(
            FixPath(L"VertexShader.cso").c_str(),
            vertex_shader_bytecode.GetAddressOf()
        );

        D3DReadFileToBlob(
            FixPath(L"PixelShader.cso").c_str(),
            pixel_shader_bytecode.GetAddressOf()
        );
    }

    // input layout
    std::vector<D3D12_INPUT_ELEMENT_DESC> input_elements = vertex_get_input_elements();

    // root signature
    {
        D3D12_DESCRIPTOR_RANGE cbv_range_transform = {};
        cbv_range_transform.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
        cbv_range_transform.NumDescriptors = 1;
        cbv_range_transform.BaseShaderRegister = 0;
        cbv_range_transform.RegisterSpace = 0;
        cbv_range_transform.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

        D3D12_ROOT_PARAMETER transform_param = {};
        transform_param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
        transform_param.ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
        transform_param.DescriptorTable.NumDescriptorRanges = 1;
        transform_param.DescriptorTable.pDescriptorRanges = &cbv_range_transform;

        D3D12_DESCRIPTOR_RANGE cbv_range_material = {};
        cbv_range_material.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
        cbv_range_material.NumDescriptors = 1;
        cbv_range_material.BaseShaderRegister = 0;
        cbv_range_material.RegisterSpace = 0;
        cbv_range_material.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

        D3D12_ROOT_PARAMETER material_param = {};
        material_param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
        material_param.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
        material_param.DescriptorTable.NumDescriptorRanges = 1;
        material_param.DescriptorTable.pDescriptorRanges = &cbv_range_material;

        D3D12_DESCRIPTOR_RANGE texture_heap_range = {};
        texture_heap_range.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
        texture_heap_range.NumDescriptors = Graphics::MAX_TEXTURE_DESCRIPTORS;
        texture_heap_range.BaseShaderRegister = 0;
        texture_heap_range.RegisterSpace = 1;
        texture_heap_range.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

        D3D12_ROOT_PARAMETER texture_heap_param = {};
        texture_heap_param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
        texture_heap_param.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
        texture_heap_param.DescriptorTable.NumDescriptorRanges = 1;
        texture_heap_param.DescriptorTable.pDescriptorRanges = &texture_heap_range;

        std::vector<D3D12_ROOT_PARAMETER> root_params = {
            transform_param,
            material_param,
            texture_heap_param
        };

        D3D12_STATIC_SAMPLER_DESC aniso_wrap_sampler = {};
        aniso_wrap_sampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        aniso_wrap_sampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        aniso_wrap_sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        aniso_wrap_sampler.Filter = D3D12_FILTER_ANISOTROPIC;
        aniso_wrap_sampler.MaxAnisotropy = 16;
        aniso_wrap_sampler.MaxLOD = D3D12_FLOAT32_MAX;
        aniso_wrap_sampler.ShaderRegister = 0; // register(s0)
        aniso_wrap_sampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

        std::vector<D3D12_STATIC_SAMPLER_DESC> samplers = {
            aniso_wrap_sampler
        };

        D3D12_ROOT_SIGNATURE_DESC root_sig_desc = {};
        // root_sig_desc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
        //                       D3D12_ROOT_SIGNATURE_FLAG_CBV_SRV_UAV_HEAP_DIRECTLY_INDEXED;
        root_sig_desc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
        root_sig_desc.NumParameters = static_cast<UINT>(root_params.size());
        root_sig_desc.pParameters = root_params.data();
        root_sig_desc.NumStaticSamplers = static_cast<UINT>(samplers.size());
        root_sig_desc.pStaticSamplers = samplers.data();

        ID3DBlob* serialized_root_sig = nullptr;
        ID3DBlob* errors = nullptr;

        HRESULT result = D3D12SerializeRootSignature(
            &root_sig_desc,
            D3D_ROOT_SIGNATURE_VERSION_1,
            &serialized_root_sig,
            &errors
        );

        if (errors != nullptr) {
            OutputDebugString((wchar_t*)errors->GetBufferPointer());
        }

        Graphics::Device->CreateRootSignature(
            0,
            serialized_root_sig->GetBufferPointer(),
            serialized_root_sig->GetBufferSize(),
            IID_PPV_ARGS(root_signature.GetAddressOf())
        );
    }

    // pipeline state
    {
        D3D12_GRAPHICS_PIPELINE_STATE_DESC pso_desc = {};

        pso_desc.InputLayout.NumElements = (uint32_t)input_elements.size();
        pso_desc.InputLayout.pInputElementDescs = input_elements.data();
        pso_desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

        pso_desc.pRootSignature = root_signature.Get();

        pso_desc.VS.pShaderBytecode = vertex_shader_bytecode->GetBufferPointer();
        pso_desc.VS.BytecodeLength = vertex_shader_bytecode->GetBufferSize();
        pso_desc.PS.pShaderBytecode = pixel_shader_bytecode->GetBufferPointer();
        pso_desc.PS.BytecodeLength = pixel_shader_bytecode->GetBufferSize();

        pso_desc.NumRenderTargets = 1;
        pso_desc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
        pso_desc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
        pso_desc.SampleDesc.Count = 1;
        pso_desc.SampleDesc.Quality = 0;

        pso_desc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
        pso_desc.RasterizerState.CullMode = D3D12_CULL_MODE_BACK;
        pso_desc.RasterizerState.DepthClipEnable = true;

        pso_desc.DepthStencilState.DepthEnable = true;
        pso_desc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
        pso_desc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;

        pso_desc.BlendState.RenderTarget[0].SrcBlend = D3D12_BLEND_ONE;
        pso_desc.BlendState.RenderTarget[0].DestBlend = D3D12_BLEND_ZERO;
        pso_desc.BlendState.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
        pso_desc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

        pso_desc.SampleMask = 0xFFFFFFFF;

        Graphics::Device->CreateGraphicsPipelineState(
            &pso_desc,
            IID_PPV_ARGS(pipeline_state.GetAddressOf())
        );
    }

    // viewport & scissor rects
    {
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
    }
}

// --------------------------------------------------------
// Initializes scene objects like cameras and entities
// --------------------------------------------------------
void Game::SceneInit() {
    camera = std::make_unique<Camera>(
        DirectX::XMFLOAT3(0.0f, 0.0f, -10.0f),
        5.0f,
        0.005f,
        XM_PIDIV2,
        Window::AspectRatio(),
        0.01f,
        100.0f
    );

    std::shared_ptr<Material> material = std::make_shared<Material>(pipeline_state);
    {
        material->AddTexture(Graphics::LoadTexture(FixPath(L"../../Assets/Textures/cobblestone_albedo.png").c_str()));
        material->AddTexture(Graphics::LoadTexture(FixPath(L"../../Assets/Textures/cobblestone_metal.png").c_str()));
        material->AddTexture(Graphics::LoadTexture(FixPath(L"../../Assets/Textures/cobblestone_normals.png").c_str()));
        material->AddTexture(Graphics::LoadTexture(FixPath(L"../../Assets/Textures/cobblestone_roughness.png").c_str()));
    }

    entities.emplace_back(
        Mesh::Load(FixPath("../../Assets/Meshes/cube.obj").c_str()),
        material
    );
    entities.emplace_back(
        Mesh::Load(FixPath("../../Assets/Meshes/helix.obj").c_str()),
        material,
        Transform({4.0f, 0.0f, 0.0f})
    );
    entities.emplace_back(
        Mesh::Load(FixPath("../../Assets/Meshes/sphere.obj").c_str()),
        material,
        Transform({-4.0f, 0.0f, 0.0f})
    );
}

// --------------------------------------------------------
// Handle resizing to match the new window size
//  - Eventually, we'll want to update our 3D camera
// --------------------------------------------------------
void Game::OnResize() {
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
    // Example input checking: Quit if the escape key is pressed
    if (Input::KeyDown(VK_ESCAPE))
        Window::Quit();

    camera->Update(deltaTime);

    for (auto& entity : entities) {
        entity.get_transform().Rotate(0, deltaTime, 0);
    }
}

// --------------------------------------------------------
// Clear the screen, redraw everything, present to the user
// --------------------------------------------------------
void Game::Draw(float deltaTime, float totalTime) {
    ClearPrevFrame();

    // our actual rendering things happen between clearing and presenting !!!!!

    Graphics::CommandList->SetGraphicsRootSignature(root_signature.Get());

    // set parameters & objects & references & etc for upcoming draw
    Graphics::CommandList->OMSetRenderTargets(
        1,
        &Graphics::RTVHandles[Graphics::get_swap_chain_index()],
        true,
        &Graphics::DSVHandle
    );
    Graphics::CommandList->RSSetViewports(1, &viewport);
    Graphics::CommandList->RSSetScissorRects(1, &scissor_rect);
    Graphics::CommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    Graphics::CommandList->SetDescriptorHeaps(1, Graphics::CBVSRVDescriptorHeap.GetAddressOf());

    // make sure to actually set the descriptor table for the start of the
    //   texture partition of the descriptor heap
    Graphics::CommandList->SetGraphicsRootDescriptorTable(2, Graphics::get_texture_heap_handle());

    for (auto& entity : entities) {
        std::shared_ptr<Mesh> mesh = entity.get_mesh();
        std::shared_ptr<Material> material = entity.get_material();

        // transform buffer
        {
            TransformBuffer data = {};
            data.world = entity.get_transform().GetWorldMatrix();
            data.view = camera->GetView();
            data.proj = camera->GetProjection();
            data.wit = entity.get_transform().GetWorldInverseTransposeMatrix();

            D3D12_GPU_DESCRIPTOR_HANDLE handle = Graphics::CBHeapFillNext(&data, sizeof(data));
            Graphics::CommandList->SetGraphicsRootDescriptorTable(0, handle);
        }

        // material buffer
        {
            MaterialBuffer data = {};
            uint32_t texture_count = material->get_texture_index_count();
            memcpy(
                data.packed_texture_indices,
                material->get_texture_indices(),
                sizeof(uint32_t) * texture_count
            );
            data.texture_index_count = texture_count;

            D3D12_GPU_DESCRIPTOR_HANDLE handle = Graphics::CBHeapFillNext(&data, sizeof(data));
            Graphics::CommandList->SetGraphicsRootDescriptorTable(1, handle);
        }

        Graphics::CommandList->SetPipelineState(material->get_pipeline_state().Get());

        D3D12_VERTEX_BUFFER_VIEW vb_view = mesh->get_vb_view();
        Graphics::CommandList->IASetVertexBuffers(0, 1, &vb_view);
        D3D12_INDEX_BUFFER_VIEW ib_view = mesh->get_ib_view();
        Graphics::CommandList->IASetIndexBuffer(&ib_view);

        Graphics::CommandList->DrawIndexedInstanced(mesh->get_index_count(), 1, 0, 0, 0);
    }

    Present();
}

void Game::ClearPrevFrame() {
    auto current_back_buffer = Graphics::BackBuffers[Graphics::get_swap_chain_index()];

    // transition layout from present to render target layout
    D3D12_RESOURCE_BARRIER rb = {};
    rb.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    rb.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    rb.Transition.pResource = current_back_buffer.Get();
    rb.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
    rb.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
    rb.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    Graphics::CommandList->ResourceBarrier(1, &rb);

    // clear main render target
    // CORNFLOWER BLUE IS BACK!!!!! oh how i miss you monogame
    float color[] = {0.4f, 0.6f, 0.75f, 1.0f};
    Graphics::CommandList->ClearRenderTargetView(
        Graphics::RTVHandles[Graphics::get_swap_chain_index()],
        color,
        0,      // no scissor rects
        nullptr // no scissor rects
    );

    // clear depth buffer
    Graphics::CommandList->ClearDepthStencilView(
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

    // transition back to present state
    D3D12_RESOURCE_BARRIER rb = {};
    rb.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    rb.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    rb.Transition.pResource = current_back_buffer.Get();
    rb.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
    rb.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
    rb.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    Graphics::CommandList->ResourceBarrier(1, &rb);

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
    Graphics::WaitForGPU();
    Graphics::ResetAllocatorAndCommandList();
}

