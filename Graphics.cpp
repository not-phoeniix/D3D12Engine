#include "Graphics.h"
#include <dxgi1_6.h>
#include <vector>

// Tell the drivers to use high-performance GPU in multi-GPU systems (like laptops)
extern "C" {
__declspec(dllexport) DWORD NvOptimusEnablement = 0x00000001;       // NVIDIA
__declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1; // AMD
}

namespace Graphics {
    // Annonymous namespace to hold variables
    // only accessible in this file
    namespace {
        bool apiInitialized = false;
        bool supportsTearing = false;
        bool vsyncDesired = false;
        BOOL isFullscreen = false;

        uint32_t back_buffer_index = 0;

        D3D_FEATURE_LEVEL featureLevel {};

        // descriptor heap management
        size_t cbvsrv_descriptor_heap_increment_size = 0;
        uint32_t cbv_descriptor_index = 0;

        // cb upload heap management
        uint64_t cb_upload_heap_size = 0;
        uint64_t cb_upload_heap_offset = 0;
        void* cb_upload_heap_start = nullptr;
    }
}

// Getters
bool Graphics::get_vsync_state() { return vsyncDesired || !supportsTearing || isFullscreen; }
std::wstring Graphics::get_api_name() {
    switch (featureLevel) {
        case D3D_FEATURE_LEVEL_10_0: return L"D3D10";
        case D3D_FEATURE_LEVEL_10_1: return L"D3D10.1";
        case D3D_FEATURE_LEVEL_11_0: return L"D3D11";
        case D3D_FEATURE_LEVEL_11_1: return L"D3D11.1";
        case D3D_FEATURE_LEVEL_12_0: return L"D3D12";
        case D3D_FEATURE_LEVEL_12_1: return L"D3D12.1";
        case D3D_FEATURE_LEVEL_12_2: return L"D3D12.2";
        default: return L"Unknown";
    }
}

uint32_t Graphics::get_swap_chain_index() {
    return back_buffer_index;
}

// --------------------------------------------------------
// Initializes the Graphics API, which requires window details.
//
// windowWidth     - Width of the window (and our viewport)
// windowHeight    - Height of the window (and our viewport)
// windowHandle    - OS-level handle of the window
// vsyncIfPossible - Sync to the monitor's refresh rate if available?
// --------------------------------------------------------
HRESULT Graphics::Initialize(unsigned int windowWidth, unsigned int windowHeight, HWND windowHandle, bool vsyncIfPossible) {
    if (apiInitialized) {
        return E_FAIL;
    }

    vsyncDesired = vsyncIfPossible;

    // determine if screen tearing is possible
    Microsoft::WRL::ComPtr<IDXGIFactory5> factory;
    if (SUCCEEDED(CreateDXGIFactory1(IID_PPV_ARGS(&factory)))) {
        BOOL tearing_supported = false;
        HRESULT feature_check = factory->CheckFeatureSupport(
            DXGI_FEATURE_PRESENT_ALLOW_TEARING,
            &tearing_supported,
            sizeof(tearing_supported)
        );

        supportsTearing = SUCCEEDED(feature_check) && tearing_supported;
    }

#if defined(DEBUG) || defined(_DEBUG)
    // debug mode setup stuff :D
    ID3D12Debug* debug_controller;
    D3D12GetDebugInterface(IID_PPV_ARGS(&debug_controller));
    debug_controller->EnableDebugLayer();
#endif

    // creating the D3D12 device !!!!
    {
        // make the device first !!!!
        HRESULT create_result = D3D12CreateDevice(
            nullptr,                            // we're not manually specifying an adapter
            D3D_FEATURE_LEVEL_11_0,             // min feature level, not desired
            IID_PPV_ARGS(Device.GetAddressOf()) // grab ID of the output device
        );
        if (FAILED(create_result)) {
            return create_result;
        }

        // determine the max feature level of the device we chose !!!
        std::vector<D3D_FEATURE_LEVEL> target_levels = {
            D3D_FEATURE_LEVEL_11_0,
            D3D_FEATURE_LEVEL_11_1,
            D3D_FEATURE_LEVEL_12_0,
            D3D_FEATURE_LEVEL_12_1,
            D3D_FEATURE_LEVEL_12_2
        };

        D3D12_FEATURE_DATA_FEATURE_LEVELS levels = {};
        levels.pFeatureLevelsRequested = target_levels.data();
        levels.NumFeatureLevels = (UINT)target_levels.size();

        Device->CheckFeatureSupport(
            D3D12_FEATURE_FEATURE_LEVELS,
            &levels,
            sizeof(D3D12_FEATURE_DATA_FEATURE_LEVELS)
        );

        featureLevel = levels.MaxSupportedFeatureLevel;
    }

#if defined(DEBUG) || defined(_DEBUG)
    // set callback for info messages !!!!
    Device->QueryInterface(IID_PPV_ARGS(&InfoQueue));
#endif

    // COMMAND ALLOCATORS AND QUEUES AND POOLS AND oh wait pools are vulkan nvm
    {
        // allocator
        Device->CreateCommandAllocator(
            D3D12_COMMAND_LIST_TYPE_DIRECT,
            IID_PPV_ARGS(CommandAllocator.GetAddressOf())
        );

        // queue
        D3D12_COMMAND_QUEUE_DESC queue_desc = {};
        queue_desc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
        queue_desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
        Device->CreateCommandQueue(
            &queue_desc, IID_PPV_ARGS(CommandQueue.GetAddressOf())
        );

        // list
        Device->CreateCommandList(
            0,                              // which GPU will this be attached to (0 for single GPU setup)
            D3D12_COMMAND_LIST_TYPE_DIRECT, // type of list to make
            CommandAllocator.Get(),         // allocator to allocate commands in !!!
            nullptr,                        // pipeline state (we don't have one yet <3)
            IID_PPV_ARGS(CommandList.GetAddressOf())
        );
    }

    // HOOOLY SWAP CHAIN !!!
    {
        DXGI_SWAP_CHAIN_DESC swap_desc = {};
        swap_desc.BufferCount = NUM_BACK_BUFFERS;
        swap_desc.BufferDesc.Width = windowWidth;
        swap_desc.BufferDesc.Height = windowHeight;
        swap_desc.BufferDesc.RefreshRate.Numerator = 60;
        swap_desc.BufferDesc.RefreshRate.Denominator = 1;
        swap_desc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        swap_desc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
        swap_desc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
        swap_desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        swap_desc.Flags = supportsTearing ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0;
        swap_desc.OutputWindow = windowHandle;
        swap_desc.SampleDesc.Count = 1;
        swap_desc.SampleDesc.Quality = 0;
        swap_desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
        swap_desc.Windowed = true;

        Microsoft::WRL::ComPtr<IDXGIFactory> dxgi_factory;
        CreateDXGIFactory(IID_PPV_ARGS(dxgi_factory.GetAddressOf()));
        HRESULT swap_result = dxgi_factory->CreateSwapChain(
            CommandQueue.Get(),
            &swap_desc,
            SwapChain.GetAddressOf()
        );
        if (FAILED(swap_result)) {
            return swap_result;
        }
    }

    // create sync things !!!
    {
        Device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(Fence.GetAddressOf()));
        FenceEvent = CreateEventEx(0, 0, 0, EVENT_ALL_ACCESS);
        FenceCounter = 0;
    }

    // we're done with all the basic API stuff
    apiInitialized = true;

    // finally we make the descriptor heaps !!!
    //   (not essential to API so we do it after that init flag is set to true)
    {
        D3D12_DESCRIPTOR_HEAP_DESC rtv_heap_desc = {};
        rtv_heap_desc.NumDescriptors = NUM_BACK_BUFFERS;
        rtv_heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
        Device->CreateDescriptorHeap(&rtv_heap_desc, IID_PPV_ARGS(RTVHeap.GetAddressOf()));

        D3D12_DESCRIPTOR_HEAP_DESC dsv_heap_desc = {};
        dsv_heap_desc.NumDescriptors = NUM_BACK_BUFFERS;
        dsv_heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
        Device->CreateDescriptorHeap(&dsv_heap_desc, IID_PPV_ARGS(DSVHeap.GetAddressOf()));
    }

    // create initial buffers at the end of setup
    ResizeBuffers(windowWidth, windowHeight);

    // create ring buffer things!
    {
        // upload heap
        {
            cb_upload_heap_size = (uint64_t)MAX_CBUFFERS * 256;

            D3D12_RESOURCE_DESC desc = {};
            desc.Alignment = 0;
            desc.DepthOrArraySize = 1;
            desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
            desc.Flags = D3D12_RESOURCE_FLAG_NONE;
            desc.Format = DXGI_FORMAT_UNKNOWN;
            desc.Width = cb_upload_heap_size,
            desc.Height = 1; // assuming this is a regular buffer and not a tex
            desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
            desc.MipLevels = 1;
            desc.SampleDesc.Count = 1;
            desc.SampleDesc.Quality = 0;

            D3D12_HEAP_PROPERTIES heap_props = {};
            heap_props.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
            heap_props.CreationNodeMask = 1;
            heap_props.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
            heap_props.Type = D3D12_HEAP_TYPE_UPLOAD;
            heap_props.VisibleNodeMask = 1;

            Device->CreateCommittedResource(
                &heap_props,
                D3D12_HEAP_FLAG_NONE,
                &desc,
                D3D12_RESOURCE_STATE_GENERIC_READ,
                nullptr,
                IID_PPV_ARGS(CBUploadHeap.GetAddressOf())
            );

            D3D12_RANGE range = {0, 0};
            CBUploadHeap->Map(0, &range, &cb_upload_heap_start);
        }

        // descriptor heap
        {
            D3D12_DESCRIPTOR_HEAP_DESC desc = {};
            desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
            desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
            desc.NumDescriptors = MAX_CBUFFERS;

            Device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(CBVSRVDescriptorHeap.GetAddressOf()));

            cbvsrv_descriptor_heap_increment_size =
                (size_t)Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
        }
    }

    // aaaaaand wait for all the background stuff to be done
    //   and return a thumbs up status
    WaitForGPU();
    return S_OK;
}

// --------------------------------------------------------
// Called at the end of the program to clean up any
// graphics API specific memory.
//
// This exists for completeness since D3D objects generally
// use ComPtrs, which get cleaned up automatically.  Other
// APIs might need more explicit clean up.
// --------------------------------------------------------
void Graphics::ShutDown() {
}

// --------------------------------------------------------
// When the window is resized, the underlying
// buffers (textures) must also be resized to match.
//
// If we don't do this, the window size and our rendering
// resolution won't match up.  This can result in odd
// stretching/skewing.
//
// width  - New width of the window (and our viewport)
// height - New height of the window (and our viewport)
// --------------------------------------------------------
void Graphics::ResizeBuffers(unsigned int width, unsigned int height) {
    if (!apiInitialized) {
        return;
    }

    WaitForGPU();

    // release resources from smart pointers with reset
    for (uint32_t i = 0; i < NUM_BACK_BUFFERS; i++) {
        BackBuffers[i].Reset();
    }

    // resize the swapchain itself
    SwapChain->ResizeBuffers(
        NUM_BACK_BUFFERS,
        width,
        height,
        DXGI_FORMAT_R8G8B8A8_UNORM,
        supportsTearing ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0
    );

    // re-grab the pointers to the handles (and recreate the views)
    SIZE_T rtv_descriptor_size = (SIZE_T)Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    for (uint32_t i = 0; i < NUM_BACK_BUFFERS; i++) {
        // store address of each back buffer
        SwapChain->GetBuffer(i, IID_PPV_ARGS(BackBuffers[i].GetAddressOf()));

        // pointer math to get correct offsets for each respective descriptor !
        RTVHandles[i] = RTVHeap->GetCPUDescriptorHandleForHeapStart();
        RTVHandles[i].ptr += rtv_descriptor_size * (SIZE_T)i;

        // create our views !!!!
        Device->CreateRenderTargetView(BackBuffers[i].Get(), 0, RTVHandles[i]);
    }

    // now we recreate the depth buffer !
    {
        DepthBuffer.Reset();

        // depth create info
        D3D12_RESOURCE_DESC depth_desc = {};
        depth_desc.Alignment = 0;
        depth_desc.DepthOrArraySize = 1;
        depth_desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        depth_desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
        depth_desc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
        depth_desc.Width = width;
        depth_desc.Height = height;
        depth_desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
        depth_desc.MipLevels = 1;
        depth_desc.SampleDesc.Count = 1;
        depth_desc.SampleDesc.Quality = 0;

        // clear value of depth stencil
        D3D12_CLEAR_VALUE clear_value = {};
        clear_value.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
        clear_value.DepthStencil.Depth = 1.0f;
        clear_value.DepthStencil.Stencil = 0;

        // describe heap that the resource will live in <3
        D3D12_HEAP_PROPERTIES heap_props = {};
        heap_props.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
        heap_props.CreationNodeMask = 1;
        heap_props.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
        heap_props.Type = D3D12_HEAP_TYPE_DEFAULT;
        heap_props.VisibleNodeMask = 1;

        // make the resource itself !!!
        Device->CreateCommittedResource(
            &heap_props,
            D3D12_HEAP_FLAG_NONE,
            &depth_desc,
            D3D12_RESOURCE_STATE_DEPTH_WRITE,
            &clear_value,
            IID_PPV_ARGS(DepthBuffer.GetAddressOf())
        );

        // aaaaand finally create the view for our new resource
        DSVHandle = DSVHeap->GetCPUDescriptorHandleForHeapStart();
        Device->CreateDepthStencilView(
            DepthBuffer.Get(),
            0,
            DSVHandle
        );
    }

    // reset/grab states
    back_buffer_index = 0;
    SwapChain->GetFullscreenState(&isFullscreen, nullptr);

    // wait for GPU to finish work <3
    WaitForGPU();
}

void Graphics::AdvanceSwapChainIndex() {
    back_buffer_index = (back_buffer_index + 1) % NUM_BACK_BUFFERS;
}

Microsoft::WRL::ComPtr<ID3D12Resource> Graphics::CreateStaticBuffer(size_t data_stride, uint32_t data_count, const void* data) {
    // create local allocator and list for one-time use
    Microsoft::WRL::ComPtr<ID3D12CommandAllocator> command_allocator;
    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> command_list;
    Device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(command_allocator.GetAddressOf()));
    Device->CreateCommandList(
        0,
        D3D12_COMMAND_LIST_TYPE_DIRECT,
        command_allocator.Get(),
        nullptr,
        IID_PPV_ARGS(command_list.GetAddressOf())
    );

    D3D12_RESOURCE_DESC desc = {};
    desc.Alignment = 0;
    desc.DepthOrArraySize = 1;
    desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    desc.Flags = D3D12_RESOURCE_FLAG_NONE;
    desc.Format = DXGI_FORMAT_UNKNOWN;
    desc.Width = data_stride * data_count;
    desc.Height = 1; // assuming this is a regular buffer and not a tex
    desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    desc.MipLevels = 1;
    desc.SampleDesc.Count = 1;
    desc.SampleDesc.Quality = 0;

    // create our output buffer
    Microsoft::WRL::ComPtr<ID3D12Resource> output_buffer;
    {
        D3D12_HEAP_PROPERTIES heap_props = {};
        heap_props.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
        heap_props.CreationNodeMask = 1;
        heap_props.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
        heap_props.Type = D3D12_HEAP_TYPE_DEFAULT;
        heap_props.VisibleNodeMask = 1;

        Device->CreateCommittedResource(
            &heap_props,
            D3D12_HEAP_FLAG_NONE,
            &desc,
            D3D12_RESOURCE_STATE_COMMON,
            nullptr,
            IID_PPV_ARGS(output_buffer.GetAddressOf())
        );
    }

    // create our staging/upload buffer
    Microsoft::WRL::ComPtr<ID3D12Resource> upload_buffer;
    {
        D3D12_HEAP_PROPERTIES heap_props = {};
        heap_props.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
        heap_props.CreationNodeMask = 1;
        heap_props.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
        heap_props.Type = D3D12_HEAP_TYPE_UPLOAD;
        heap_props.VisibleNodeMask = 1;

        Device->CreateCommittedResource(
            &heap_props,
            D3D12_HEAP_FLAG_NONE,
            &desc,
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(upload_buffer.GetAddressOf())
        );
    }

    // map buffer, copy data
    void* mapped = nullptr;
    upload_buffer->Map(0, nullptr, &mapped);
    memcpy(mapped, data, data_stride * data_count);
    upload_buffer->Unmap(0, nullptr);

    // copy staging buffer data to return buffer
    command_list->CopyResource(output_buffer.Get(), upload_buffer.Get());

    // transition return buffer state to one that's optimal for shader reading
    D3D12_RESOURCE_BARRIER barrier = {};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    barrier.Transition.pResource = output_buffer.Get();
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_GENERIC_READ;
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    command_list->ResourceBarrier(1, &barrier);

    // close and submit command list for execution
    command_list->Close();
    ID3D12CommandList* list = command_list.Get();
    CommandQueue->ExecuteCommandLists(1, &list);

    // wait for work to finish and return our new buffer :D
    WaitForGPU();
    return output_buffer;
}

D3D12_GPU_DESCRIPTOR_HANDLE Graphics::CBHeapFillNext(const void* data, size_t size) {
    // ensure multiples of 256
    size_t reservation_size = (size + 255) / 256 * 256;

    if (cb_upload_heap_offset + (uint64_t)reservation_size >= cb_upload_heap_size) {
        cb_upload_heap_offset = 0;
    }

    D3D12_GPU_VIRTUAL_ADDRESS virtual_gpu_addr =
        CBUploadHeap->GetGPUVirtualAddress() + cb_upload_heap_offset;

    // copy data to heap
    {
        void* dest = reinterpret_cast<void*>((char*)cb_upload_heap_start + cb_upload_heap_offset);
        memcpy(dest, data, size);
        cb_upload_heap_offset += (uint64_t)reservation_size;
    }

    // create a CBV for this section
    {
        D3D12_CPU_DESCRIPTOR_HANDLE cpu_handle = CBVSRVDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
        D3D12_GPU_DESCRIPTOR_HANDLE gpu_handle = CBVSRVDescriptorHeap->GetGPUDescriptorHandleForHeapStart();

        cpu_handle.ptr += ((uint64_t)cbv_descriptor_index * cbvsrv_descriptor_heap_increment_size);
        gpu_handle.ptr += ((uint64_t)cbv_descriptor_index * cbvsrv_descriptor_heap_increment_size);

        D3D12_CONSTANT_BUFFER_VIEW_DESC desc = {};
        desc.BufferLocation = virtual_gpu_addr;
        desc.SizeInBytes = (UINT)reservation_size;

        Device->CreateConstantBufferView(&desc, cpu_handle);

        cbv_descriptor_index++;
        cbv_descriptor_index %= MAX_CBUFFERS;

        return gpu_handle;
    }
}

void Graphics::ResetAllocatorAndCommandList() {
    CommandAllocator->Reset();
    CommandList->Reset(CommandAllocator.Get(), nullptr);
}

void Graphics::CloseAndExecuteCommandList() {
    CommandList->Close();
    ID3D12CommandList* list = CommandList.Get();
    CommandQueue->ExecuteCommandLists(1, &list);
}

void Graphics::WaitForGPU() {
    FenceCounter++;
    CommandQueue->Signal(Fence.Get(), FenceCounter);

    if (Fence->GetCompletedValue() < FenceCounter) {
        Fence->SetEventOnCompletion(FenceCounter, FenceEvent);
        WaitForSingleObject(FenceEvent, INFINITE);
    }
}

// --------------------------------------------------------
// Prints graphics debug messages waiting in the queue
// --------------------------------------------------------
void Graphics::PrintDebugMessages() {
    // Do we actually have an info queue (usually in debug mode)
    if (!InfoQueue)
        return;

    // Any messages?
    UINT64 messageCount = InfoQueue->GetNumStoredMessages();
    if (messageCount == 0)
        return;

    // Loop and print messages
    for (UINT64 i = 0; i < messageCount; i++) {
        // Get the size so we can reserve space
        size_t messageSize = 0;
        InfoQueue->GetMessage(i, 0, &messageSize);

        // Reserve space for this message
        D3D12_MESSAGE* message = (D3D12_MESSAGE*)malloc(messageSize);
        InfoQueue->GetMessage(i, message, &messageSize);

        // Print and clean up memory
        if (message) {
            // Color code based on severity
            switch (message->Severity) {
                case D3D12_MESSAGE_SEVERITY_CORRUPTION:
                case D3D12_MESSAGE_SEVERITY_ERROR:
                    printf("\x1B[91m");
                    break; // RED

                case D3D12_MESSAGE_SEVERITY_WARNING:
                    printf("\x1B[93m");
                    break; // YELLOW

                case D3D12_MESSAGE_SEVERITY_INFO:
                case D3D12_MESSAGE_SEVERITY_MESSAGE:
                    printf("\x1B[96m");
                    break; // CYAN
            }

            printf("%s\n\n", message->pDescription);
            free(message);

            // Reset color
            printf("\x1B[0m");
        }
    }

    // Clear any messages we've printed
    InfoQueue->ClearStoredMessages();
}
