#pragma comment( lib, "user32" )          // link against the win32 library
#pragma comment( lib, "d3d11.lib" )       // direct3D library
#pragma comment( lib, "dxgi.lib" )        // directx graphics interface
#pragma comment( lib, "d3dcompiler.lib" ) // shader compiler

#ifndef  _UNICODE
#def _UNICODE
#endif // ! _UNICODE
//#define WIN32_LEAN_AND_MEAN
//#include <windows.h>
//include<windef.h>
#include <d3d11_1.h>
#include <d3dcompiler.h>

#include<assert.h>
#include <math.h> // sin, cos for rotation
#include<thread>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"


#define TITLE "Minimal D3D11 by OkabeKiyouma"
#define ArrayCount(a) (sizeof(a) / sizeof((a)[0]))

#include"UserWindow.h"
#include "KKMath.h"


static bool windowDidResize = false;
bool isRunning = true;
static bool isHotReloading = false;

enum KeyMap {
    VK_KEY_NUM0  = 0x30,
    VK_KEY_NUM1  = 0x31,
    VK_KEY_NUM2  = 0x32,
    VK_KEY_NUM3  = 0x33,
    VK_KEY_NUM4  = 0x34,
    VK_KEY_NUM5  = 0x35,
    VK_KEY_NUM6  = 0x36,
    VK_KEY_NUM7  = 0x37,
    VK_KEY_NUM8  = 0x38,
    VK_KEY_NUM9  = 0x39,
    VK_KEY_A     = 0x41,
    VK_KEY_B     = 0x42,
    VK_KEY_C     = 0x43,
    VK_KEY_D     = 0x44,
    VK_KEY_E     = 0x45,
    VK_KEY_F     = 0x46,
    VK_KEY_G     = 0x47,
    VK_KEY_H     = 0x48,
    VK_KEY_I     = 0x49,
    VK_KEY_J     = 0x4A,
    VK_KEY_K     = 0x4B,
    VK_KEY_L     = 0x4C,
    VK_KEY_M     = 0x4D,
    VK_KEY_N     = 0x4E,
    VK_KEY_O     = 0x4F,
    VK_KEY_P     = 0x50,
    VK_KEY_Q     = 0x51,
    VK_KEY_R     = 0x52,
    VK_KEY_S     = 0x53,
    VK_KEY_T     = 0x54,
    VK_KEY_U     = 0x55,
    VK_KEY_V     = 0x56,
    VK_KEY_W     = 0x57,
    VK_KEY_X     = 0x58,
    VK_KEY_Y     = 0x59,
    VK_KEY_Z     = 0x5A,
};


typedef struct {
    const wchar_t* filePath;
    ID3D11Device1*& device;
    ID3D11VertexShader*& vertexShader;
    ID3DBlob*& vsBlob;
    ID3D11PixelShader*& pixelShader;
} threadArg;

//provide vertexShader pointer and Blob pointer to hold created vertexshader object and blob object
static bool MakeVertexShader(const wchar_t* filePath, ID3D11Device1*& device ,ID3D11VertexShader*& vertexShader, ID3DBlob*& vsBlob, const char* entryPoint="vs_main") {
    ID3DBlob* shaderCompileErrorsBlob;
#ifdef _DEBUG
    HRESULT hResult = D3DCompileFromFile(filePath, nullptr, nullptr, entryPoint, "vs_5_0", D3DCOMPILE_DEBUG | D3DCOMPILE_WARNINGS_ARE_ERRORS, 0, &vsBlob, &shaderCompileErrorsBlob);
#else
    HRESULT hResult = D3DCompileFromFile(filePath, nullptr, nullptr, entryPoint, "vs_5_0", D3DCOMPILE_OPTIMIZATION_LEVEL3, 0, &vsBlob, &shaderCompileErrorsBlob);
#endif
    if (FAILED(hResult))
    {
        const char* errorString = NULL;
        if (hResult == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND))
            errorString = "Could not compile shader; file not found";
        else if (shaderCompileErrorsBlob) {
            errorString = (const char*)shaderCompileErrorsBlob->GetBufferPointer();
            shaderCompileErrorsBlob->Release();
        }
        MessageBoxA(0, errorString, "Shader Compiler Error", MB_ICONERROR | MB_OK);
        return false;
    }
    
    hResult = device->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr, &vertexShader);
    assert(SUCCEEDED(hResult));
    //vsBlob->Release();  //not released because used in input layout
    return true;
}


static bool MakePixelShader(const wchar_t* filePath, ID3D11Device1*& device, ID3D11PixelShader*& pixelShader, const char* entryPoint="ps_main") {
    ID3DBlob* psBlob;
    ID3DBlob* shaderCompileErrorsBlob= NULL;
    HRESULT hResult = D3DCompileFromFile(filePath, nullptr, nullptr, entryPoint, "ps_5_0", 0, 0, &psBlob, &shaderCompileErrorsBlob);
    if (FAILED(hResult))
    {
        const char* errorString = NULL;
        if (hResult == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND))
            errorString = "Could not compile shader; file not found";
        else if (shaderCompileErrorsBlob) {
            errorString = (const char*)shaderCompileErrorsBlob->GetBufferPointer();
            shaderCompileErrorsBlob->Release();
        }
        MessageBoxA(0, errorString, "Shader Compiler Error", MB_ICONERROR | MB_OK);
        return false;
    }

    hResult = device->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, &pixelShader);
    assert(SUCCEEDED(hResult));
    psBlob->Release();
    return true;
}

DWORD hasDirectoryChanged(void* thrdarg) {
    threadArg* thrdArg = (threadArg*)thrdarg;
    DWORD nBufferLength = 200;
    wchar_t* watchDir = new wchar_t[200];
    GetCurrentDirectory(nBufferLength, watchDir);
    size_t dirLength = wcslen(watchDir);
    for (int i = 0; i < dirLength; ++i) {
        if (watchDir[i] == '\\')
            watchDir[i] = '/';
    }
    wcscat_s(watchDir, 200, L"/res/shader");

    HANDLE fileChangeNotif = FindFirstChangeNotificationW(watchDir, TRUE, FILE_NOTIFY_CHANGE_LAST_WRITE);
    if (fileChangeNotif == INVALID_HANDLE_VALUE) {
        OutputDebugStringW(L"\nERROR: FindFirstChangeNotification function failed.\n");
        ExitProcess(GetLastError());
    }
    DWORD waitState;
    while (1) {
        waitState = WaitForSingleObjectEx(fileChangeNotif, INFINITE, true);

        switch (waitState) {
        case WAIT_OBJECT_0: {
            FILE* file=NULL;
            _InterlockedIncrement((long*) & isHotReloading);
            while (_wfopen_s(&file, thrdArg->filePath, L"r")!=0);
            fclose(file);
            MakeVertexShader(thrdArg->filePath, thrdArg->device, thrdArg->vertexShader, thrdArg->vsBlob);
            file = NULL;
            while (_wfopen_s(&file, thrdArg->filePath, L"r")!=0);
            fclose(file);
            MakePixelShader(thrdArg->filePath, thrdArg->device, thrdArg->pixelShader);
            thrdArg->vsBlob->Release();
            _InterlockedDecrement((long*) & isHotReloading);
            FindNextChangeNotification(fileChangeNotif);
        }
        break;
        default:
            break;
        }
    }
}


LRESULT CALLBACK WinProc(HWND hWindow, UINT uMsg, WPARAM wParam, LPARAM lParam);

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow) {

    UserWindow mainWin(L"Parent");
    mainWin.WindowProc = WinProc;
    if (!mainWin.init(L"Win32 Test", 0, WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,800,800))
        return 0;

    ShowWindow(mainWin.Window, nCmdShow);
    
    //DX11

    
//Device Setup and its context
    D3D_FEATURE_LEVEL featureLevels[] =
    {
        D3D_FEATURE_LEVEL_11_1,
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_1,
        D3D_FEATURE_LEVEL_10_0,
        D3D_FEATURE_LEVEL_9_3,
        D3D_FEATURE_LEVEL_9_1
    };
    ID3D11Device* baseDevice;
    ID3D11DeviceContext* baseDeviceContext;
    UINT creationFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
#if defined(_DEBUG)
    creationFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    //hResult solely stores result and nothing else
    HRESULT hResult = D3D11CreateDevice(nullptr, 
        D3D_DRIVER_TYPE_HARDWARE, 
        nullptr, 
        creationFlags, 
        featureLevels, 
        ARRAYSIZE(featureLevels), 
        D3D11_SDK_VERSION,
        &baseDevice, 
        nullptr, 
        &baseDeviceContext);
    
    if (FAILED(hResult)) {
        MessageBoxA(0, "D3D11CreateDevice() failed", "Fatal Error", MB_OK);
        return GetLastError();
    }

    // Set 1.1 interface of D3D11 Device and Context
    ID3D11Device1* device;
    hResult = baseDevice->QueryInterface(__uuidof(ID3D11Device1), reinterpret_cast<void**>(&device));
    assert(SUCCEEDED(hResult));
    baseDevice->Release();

    ID3D11DeviceContext1* deviceContext;
    hResult = baseDeviceContext->QueryInterface(__uuidof(ID3D11DeviceContext1), reinterpret_cast<void**>(&deviceContext));
    assert(SUCCEEDED(hResult));
    baseDeviceContext->Release();



//Swap chain setup

    //To set dxgi factory needed for swapchain
    IDXGIDevice1* dxgiDevice;
    hResult = device->QueryInterface(__uuidof(IDXGIDevice1), reinterpret_cast<void**>(&dxgiDevice));
    assert(SUCCEEDED(hResult));

    IDXGIAdapter* dxgiAdapter;
    hResult = dxgiDevice->GetAdapter(&dxgiAdapter);
    assert(SUCCEEDED(hResult));
    dxgiDevice->Release();

    DXGI_ADAPTER_DESC adapterDesc;
    dxgiAdapter->GetDesc(&adapterDesc);

    OutputDebugStringA("Graphics Device: ");
    OutputDebugStringW(adapterDesc.Description);

    IDXGIFactory2* dxgiFactory;
    hResult = dxgiAdapter->GetParent(__uuidof(IDXGIFactory2), reinterpret_cast<void**>(&dxgiFactory));
    assert(SUCCEEDED(hResult));
    dxgiAdapter->Release();

    //swap chain 
    DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
    swapChainDesc.Width = 0; // use window width
    swapChainDesc.Height = 0; // use window height
    swapChainDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;   //bit-blt model
    //swapChainDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;   //flip model
    swapChainDesc.Stereo = FALSE;
    swapChainDesc.SampleDesc.Count = 1;
    swapChainDesc.SampleDesc.Quality = 0;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.BufferCount = 2;
    swapChainDesc.Scaling = DXGI_SCALING_STRETCH;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD; //bit-blt model 
    //swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL; //flip model
    swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
    swapChainDesc.Flags = 0;

    IDXGISwapChain1* swapChain;
    hResult = dxgiFactory->CreateSwapChainForHwnd(device, mainWin.Window, &swapChainDesc, nullptr, nullptr, &swapChain);
    assert(SUCCEEDED(hResult));
    dxgiFactory->Release();

#if defined(_DEBUG)
    ID3D11Debug* debug = 0;

    if (SUCCEEDED(device->QueryInterface(__uuidof(ID3D11Debug), (void**)&debug))) {
        ID3D11InfoQueue* info_queue = nullptr;
        if (SUCCEEDED(device->QueryInterface(__uuidof(ID3D11InfoQueue), (void**)&info_queue))) {
            //DebugWriteLogEx("DirectX", "ID3D11Debug enabled.");

            info_queue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_CORRUPTION, true);
            info_queue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_ERROR, true);

            D3D11_MESSAGE_ID hide[] = { D3D11_MESSAGE_ID_SETPRIVATEDATA_CHANGINGPARAMS };

            D3D11_INFO_QUEUE_FILTER filter = {};
            filter.DenyList.NumIDs = ArrayCount(hide);
            filter.DenyList.pIDList = hide;
            info_queue->AddStorageFilterEntries(&filter);

            info_queue->Release();
        }
    }
    debug->Release();
#endif



//Create Framebuffer Render Target
    ID3D11RenderTargetView* frameBufferView;
    
    //get a back buffer from swapchain
    ID3D11Texture2D* frameBuffer;
    hResult = swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&frameBuffer);
    assert(SUCCEEDED(hResult));

    //set backbuffer as render target
    hResult = device->CreateRenderTargetView(frameBuffer, 0, &frameBufferView);
    assert(SUCCEEDED(hResult));
    frameBuffer->Release();
    
//Create shaders

    //Vertex Shader
    ID3DBlob* vsBlob;
    ID3D11VertexShader* vertexShader;
    MakeVertexShader(L"res/shader/shader.hlsl",device, vertexShader, vsBlob);

    //Pixel Shader
    ID3D11PixelShader* pixelShader;
    MakePixelShader(L"res/shader/shader.hlsl", device, pixelShader);

// Create Input Layout
    ID3D11InputLayout* inputLayout;
    {
        D3D11_INPUT_ELEMENT_DESC inputElementDesc[] =
        {
            { "POS", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "COL", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "TEX", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        };

        HRESULT hResult = device->CreateInputLayout(inputElementDesc, ARRAYSIZE(inputElementDesc), vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), &inputLayout);
        assert(SUCCEEDED(hResult));
        vsBlob->Release();
    }


// Create Vertex Buffer
    ID3D11Buffer* vertexBuffer;
    UINT numVerts;
    UINT stride;
    UINT offset;
    {
        float vertexData[] = { 
           //  x,       y,     z,     r,     g,     b,     a,     u,     v
           -0.5f,   -0.5f,   0.f,   0.f,   0.f,   1.f,   1.f,   0.f,   1.f, 
           -0.5f,    0.5f,   0.f,   0.f,   1.f,   0.f,   1.f,   0.f,   0.f,
            0.5f,   -0.5f,   0.f,   1.f,   0.f,   0.f,   1.f,   1.f,   1.f,
            0.5f,    0.5f,   0.f,   0.f,   0.f,   1.f,   1.f,   1.f,   0.f, 
        };

        stride = 9 * sizeof(float);
        numVerts = sizeof(vertexData) / stride;
        offset = 0;

        D3D11_BUFFER_DESC vertexBufferDesc = {};
        vertexBufferDesc.ByteWidth = sizeof(vertexData);
        vertexBufferDesc.Usage     = D3D11_USAGE_IMMUTABLE;
        vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

        D3D11_SUBRESOURCE_DATA vertexSubresourceData = { vertexData };

        HRESULT hResult = device->CreateBuffer(&vertexBufferDesc, &vertexSubresourceData, &vertexBuffer);
        assert(SUCCEEDED(hResult));
    }

//Create Index Buffer
    ID3D11Buffer* indexBuffer;
    {
        UINT index[] = {
            0,1,2,
            2,1,3
        };

        D3D11_BUFFER_DESC indexBufferDesc = {};
        indexBufferDesc.ByteWidth = sizeof(index);
        indexBufferDesc.Usage = D3D11_USAGE_IMMUTABLE;
        indexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;

        D3D11_SUBRESOURCE_DATA indexData = { index };

        HRESULT hResult = device->CreateBuffer(&indexBufferDesc, &indexData, &indexBuffer);
        assert(SUCCEEDED(hResult));
    }

//Set Constant Buffer
    struct Constants
    {
        Vec2 pos;
    };

    ID3D11Buffer* constantBuffer;
    {
        D3D11_BUFFER_DESC constantBufferDesc = {};
        // ByteWidth must be a multiple of 16, per the docs
        constantBufferDesc.ByteWidth = sizeof(Constants) + 0xf & 0xfffffff0;
        constantBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
        constantBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        constantBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

        HRESULT hResult = device->CreateBuffer(&constantBufferDesc, nullptr, &constantBuffer);
        assert(SUCCEEDED(hResult));
    }

//Set Sampler State
    D3D11_SAMPLER_DESC samplerDesc = {};
    samplerDesc.Filter         = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    samplerDesc.AddressU       = D3D11_TEXTURE_ADDRESS_BORDER; //clamp,wrap,mirror,etc
    samplerDesc.AddressV       = D3D11_TEXTURE_ADDRESS_BORDER;
    samplerDesc.AddressW       = D3D11_TEXTURE_ADDRESS_BORDER;
    samplerDesc.BorderColor[0] = 1.0f;
    samplerDesc.BorderColor[1] = 1.0f;
    samplerDesc.BorderColor[2] = 1.0f;
    samplerDesc.BorderColor[3] = 1.0f;
    samplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER; //compares new data and with old data accordingly

    ID3D11SamplerState* samplerState;
    device->CreateSamplerState(&samplerDesc, &samplerState);

//Load Image
    int texWidth, texHeight, texNumChannels;
    int texForceNumChannels = 4;
    unsigned char* testTextureBytes = stbi_load("res/images/awesomeface.png", &texWidth, &texHeight, &texNumChannels, texForceNumChannels);
    assert(testTextureBytes);
    int texBytesPerRow = 4 * texWidth;

//Create Texture
    D3D11_TEXTURE2D_DESC textureDesc = {};
    textureDesc.Width            = texWidth;
    textureDesc.Height           = texHeight;
    textureDesc.MipLevels        = 1;
    textureDesc.ArraySize        = 1;
    textureDesc.Format           = DXGI_FORMAT_R8G8B8A8_UNORM;
    textureDesc.SampleDesc.Count = 1;
    textureDesc.Usage            = D3D11_USAGE_IMMUTABLE;
    textureDesc.BindFlags        = D3D11_BIND_SHADER_RESOURCE;

    D3D11_SUBRESOURCE_DATA textureSubresourceData = {};
    textureSubresourceData.pSysMem = testTextureBytes;
    textureSubresourceData.SysMemPitch = texBytesPerRow;

    ID3D11Texture2D* texture;
    device->CreateTexture2D(&textureDesc, &textureSubresourceData, &texture);

    ID3D11ShaderResourceView* textureView;
    device->CreateShaderResourceView(texture, nullptr, &textureView);

    free(testTextureBytes);

//Set up Rasterizer 
    
    //Set Rasterizer State
    ID3D11RasterizerState1* rasterizerState;
    {
        CD3D11_RASTERIZER_DESC1 rasterizerDesc = {};
        //rasterizerDesc.FillMode              = D3D11_FILL_WIREFRAME;
        rasterizerDesc.FillMode              = D3D11_FILL_SOLID;
        rasterizerDesc.CullMode              = D3D11_CULL_BACK;
        rasterizerDesc.DepthBias             = FALSE;
        rasterizerDesc.DepthBiasClamp        = 0;
        rasterizerDesc.DepthClipEnable       = TRUE;
        rasterizerDesc.SlopeScaledDepthBias  = 0;
        rasterizerDesc.FrontCounterClockwise = FALSE; //if true, vertices in counter clockwise are considered facing forward
        rasterizerDesc.ScissorEnable         = TRUE;
        rasterizerDesc.AntialiasedLineEnable = FALSE; //alphaline anti-aliasing
        rasterizerDesc.MultisampleEnable     = TRUE; //quadrilateral line anti-aliasing
        rasterizerDesc.ForcedSampleCount     = 0;
        
        HRESULT hResult = device->CreateRasterizerState1(&rasterizerDesc, &rasterizerState);
        assert(SUCCEEDED(hResult));
    }

//Set Blend Function
    ID3D11BlendState1* blendState = nullptr;
    {
        D3D11_BLEND_DESC1 blendFunctionDesc = {};
        blendFunctionDesc.AlphaToCoverageEnable                 = TRUE;
        blendFunctionDesc.RenderTarget[0].BlendEnable           = FALSE;
        blendFunctionDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

        device->CreateBlendState1(&blendFunctionDesc, &blendState);
    }

    deviceContext->OMSetBlendState( blendState, NULL, 0xffffffff);


    threadArg thrdArg = { L"res/shader/shader.hlsl",device, vertexShader, vsBlob, pixelShader };

    HANDLE waitThread = CreateThread(NULL, 0, hasDirectoryChanged, (void*)&thrdArg, 0, NULL);

    
    //Main Loop
    while (isRunning)
    {
        // Run the message loop.
        MSG msg = { };
        while (PeekMessageW(&msg, 0, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        if (windowDidResize)
        {
            deviceContext->OMSetRenderTargets(0, 0, 0);
            frameBufferView->Release();

            HRESULT res = swapChain->ResizeBuffers(0, 0, 0, DXGI_FORMAT_UNKNOWN, 0);
            assert(SUCCEEDED(res));

            ID3D11Texture2D* frameBuffer;
            res = swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&frameBuffer);
            assert(SUCCEEDED(res));

            res = device->CreateRenderTargetView(frameBuffer, NULL,
                &frameBufferView);
            assert(SUCCEEDED(res));
            frameBuffer->Release();

            windowDidResize = false;
        }

 
        D3D11_MAPPED_SUBRESOURCE mappedSubresource;
        deviceContext->Map(constantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedSubresource);
        Constants* constants = (Constants*)(mappedSubresource.pData);
        constants->pos   = { 0.5f, 0.3f };
        deviceContext->Unmap(constantBuffer, 0);

        float backgroundColor[4] = { 0.1f, 0.2f, 0.6f, 1.0f };
        deviceContext->ClearRenderTargetView(frameBufferView, backgroundColor);

        RECT winRect;
        GetClientRect(mainWin.Window, &winRect);
        D3D11_VIEWPORT viewport = { 0.0f, 0.0f, (FLOAT)(winRect.right - winRect.left), (FLOAT)(winRect.bottom - winRect.top), 0.0f, 1.0f };
        deviceContext->RSSetViewports(1, &viewport);

        //Set up Scissor Rectangle
        D3D11_RECT scissorRect = {(LONG)(viewport.TopLeftY),(LONG)(viewport.TopLeftX),(LONG)(viewport.TopLeftY+viewport.Width),(LONG)(viewport.TopLeftX+viewport.Height)};
        deviceContext->RSSetScissorRects(1, &scissorRect);

        deviceContext->OMSetRenderTargets(1, &frameBufferView, nullptr);

        while (isHotReloading) {}
        deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        deviceContext->IASetInputLayout(inputLayout);
        deviceContext->IASetVertexBuffers(0, 1, &vertexBuffer, &stride, &offset);
        deviceContext->IASetIndexBuffer(indexBuffer, DXGI_FORMAT_R32_UINT, 0);

        deviceContext->VSSetShader(vertexShader, nullptr, 0);
        deviceContext->VSSetConstantBuffers(1, 1, &constantBuffer);

        deviceContext->PSSetShader(pixelShader, nullptr, 0);
        deviceContext->PSSetShaderResources(0, 1, &textureView);
        deviceContext->PSSetSamplers(0, 1, &samplerState);

        deviceContext->RSSetState(rasterizerState);

        deviceContext->DrawIndexed(6, 0, 0);

        swapChain->Present(1, 0);
    }
    return 0;
}

LRESULT CALLBACK WinProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_KEYDOWN:
        switch (wParam) {
        case VK_ESCAPE:
            if (MessageBox(hwnd, L"Really quit?", L"My application", MB_OKCANCEL) == IDOK)
            {
                DestroyWindow(hwnd);
            }
            return 0;

        }
        return 0;

    case WM_DESTROY:
        PostQuitMessage(0);
        isRunning = false;
        return 0;

    case WM_CLOSE:
        if (MessageBox(hwnd, L"Really quit?", L"My application", MB_OKCANCEL) == IDOK)
        {
            DestroyWindow(hwnd);
        }
        // Else: User canceled. Do nothing.
        return 0;

    //case WM_PAINT:
    //{
    //    PAINTSTRUCT ps;
    //    HDC hdc = BeginPaint(hwnd, &ps);



    //    FillRect(hdc, &ps.rcPaint, (HBRUSH)(COLOR_WINDOW + 1));

    //    EndPaint(hwnd, &ps);
    //}
    case WM_SIZE:
    {
        windowDidResize = true;
        break;
    }
    return 0;

    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}