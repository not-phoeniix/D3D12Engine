#pragma once

#include <Windows.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <string>
#include <wrl/client.h>

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")

namespace Graphics {
    // --- CONSTANTS ---

    constexpr uint32_t NUM_BACK_BUFFERS = 2;
    constexpr uint32_t MAX_CBUFFERS = 1000;
    constexpr uint32_t MAX_TEXTURE_DESCRIPTORS = 100;

    // --- GLOBAL VARS ---

    // Primary D3D12 API objects
    inline Microsoft::WRL::ComPtr<ID3D12Device> Device;
    inline Microsoft::WRL::ComPtr<IDXGISwapChain> SwapChain;

    // Command stuff!
    inline Microsoft::WRL::ComPtr<ID3D12CommandAllocator> CommandAllocators[NUM_BACK_BUFFERS];
    inline Microsoft::WRL::ComPtr<ID3D12CommandQueue> CommandQueue;
    inline Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> CommandList;

    // Back buffers for swapchain
    inline Microsoft::WRL::ComPtr<ID3D12Resource> BackBuffers[NUM_BACK_BUFFERS];
    inline Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> RTVHeap;
    inline D3D12_CPU_DESCRIPTOR_HANDLE RTVHandles[NUM_BACK_BUFFERS] = {};

    // Depth buffer for depth & stencil things !!
    inline Microsoft::WRL::ComPtr<ID3D12Resource> DepthBuffer;
    inline Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> DSVHeap;
    inline D3D12_CPU_DESCRIPTOR_HANDLE DSVHandle = {};

    // cbuffer things !!!
    inline Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> CBVSRVDescriptorHeap;
    inline Microsoft::WRL::ComPtr<ID3D12Resource> CBUploadHeap;

    // Sync objects !!!
    inline Microsoft::WRL::ComPtr<ID3D12Fence> WaitFence;
    inline HANDLE WaitFenceEvent = 0;
    inline uint64_t WaitFenceCounter = 0;
    inline Microsoft::WRL::ComPtr<ID3D12Fence> FrameFence;
    inline HANDLE FrameFenceEvent = 0;
    inline uint64_t FrameFenceCounters[NUM_BACK_BUFFERS] = {};

    // Debug Layer
    inline Microsoft::WRL::ComPtr<ID3D12InfoQueue> InfoQueue;

    // --- FUNCTIONS ---

    // Getters
    bool get_vsync_state();
    std::wstring get_api_name();
    uint32_t get_swap_chain_index();
    D3D12_GPU_DESCRIPTOR_HANDLE get_texture_heap_handle();

    // General functions
    HRESULT Initialize(unsigned int windowWidth, unsigned int windowHeight, HWND windowHandle, bool vsyncIfPossible);
    void ShutDown();
    void ResizeBuffers(unsigned int width, unsigned int height);
    void AdvanceSwapChainIndex();
    Microsoft::WRL::ComPtr<ID3D12Resource> CreateStaticBuffer(size_t data_stride, uint32_t data_count, const void* data);
    D3D12_GPU_DESCRIPTOR_HANDLE CBHeapFillNext(const void* data, size_t size);
    uint32_t LoadTexture(const wchar_t* file, bool generate_mips = true);

    // Command stuff & sync
    void ResetAllocatorAndCommandList(uint32_t index);
    void CloseAndExecuteCommandList();
    void WaitForGPU();

    // Debug Layer
    void PrintDebugMessages();
}
