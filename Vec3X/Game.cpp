#include "pch.h"
#include "Game.h"
#include "vec3x_emulator_types.hpp"
#include <fstream>

CGame* GAME_INSTANCE = nullptr;

extern "C" {
    void vectrex_emulator_init(int width, int height);
    void vectrex_emulator_start(const char* romfile, const char* romname, const char* cartfile, const char* cartName);
    void vectrex_emulator_frame(void);
    void vectrex_emulator_stop(void);
    void vectrex_emulator_pause(void);
    void vectrex_emulator_resume(void);
    void vectrex_emulator_key(int vk, int pressed);

    void vectrex_add_line(int x1, int y1, int x2, int y2, uint8_t color) {
        GAME_INSTANCE->AddLine(x1, y1, x2, y2, color);
    }
}

Array<byte>^ LoadShaderFile(std::string File) {
    Array<byte>^ fileData = nullptr;

    std::ifstream VertexFile(File, std::ios::in | std::ios::binary | std::ios::ate);
    
    if (VertexFile.is_open()) {
        int Length = (int)VertexFile.tellg();

        fileData = ref new Array<byte>(Length);
        VertexFile.seekg(0, std::ios::beg);
        VertexFile.read(reinterpret_cast<char*>(fileData->Data), Length);
        VertexFile.close();
    }

    return fileData;
}

void CGame::LoadGame() {
    CoreWindow^ window = CoreWindow::GetForCurrentThread();

    std::string name = m_romList[m_selectedRom];
    if (name.size() == 0) {
        vectrex_emulator_init((int)window->Bounds.Width, (int)window->Bounds.Height);
        vectrex_emulator_start("romfast.bin", "fastrom", nullptr, nullptr);
    }
    else {
        std::string gameFile = name + ".bin";
        vectrex_emulator_init((int)window->Bounds.Width, (int)window->Bounds.Height);
        vectrex_emulator_start("romfast.bin", "fastrom", gameFile.c_str(), name.c_str());
    }
}

void CGame::NextGame() {
    m_selectedRom++;
    if (m_selectedRom >= m_romList.size())
        m_selectedRom = 0;

    LoadGame();
}

void CGame::Initialize() {
    GAME_INSTANCE = this;

    ComPtr<ID3D11Device> dev11;
    ComPtr<ID3D11DeviceContext> devcon11;

    D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0, nullptr, 0, D3D11_SDK_VERSION, &dev11, nullptr, &devcon11);
    
    dev11.As(&m_dxDevice);
    devcon11.As(&m_dxContext);

    ComPtr<IDXGIDevice1> dxgiDevice;
    m_dxDevice.As(&dxgiDevice);
    ComPtr<IDXGIAdapter> dxgiAdapter;
    dxgiDevice->GetAdapter(&dxgiAdapter);
    ComPtr<IDXGIFactory2> dxgiFactory;
    dxgiAdapter->GetParent(__uuidof(IDXGIFactory2), &dxgiFactory);

    DXGI_SWAP_CHAIN_DESC1 scd = {0};
    scd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;    // how the swap chain should be used
    scd.BufferCount = 2;                                  // a front buffer and a back buffer
    scd.Format = DXGI_FORMAT_B8G8R8A8_UNORM;              // the most common swap chain format
    scd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;    // the recommended flip mode
    scd.SampleDesc.Count = 1;                             // disable anti-aliasing

    CoreWindow^ window = CoreWindow::GetForCurrentThread();
    dxgiFactory->CreateSwapChainForCoreWindow(m_dxDevice.Get(), reinterpret_cast<IUnknown*>(window), &scd, nullptr, &m_dxSwapChain);

    ComPtr<ID3D11Texture2D> backBuffer;
    m_dxSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), &backBuffer);

    m_dxDevice->CreateRenderTargetView(backBuffer.Get(), nullptr, &m_dxRenderTarget);

    D3D11_VIEWPORT viewport = {0};
    viewport.TopLeftX = (window->Bounds.Width-window->Bounds.Height);
    viewport.TopLeftY = 0;
    viewport.Width = window->Bounds.Height*2; // Because of Retina Display * 2 (Hack)
    viewport.Height = window->Bounds.Height*2;

    m_dxContext->RSSetViewports(1, &viewport);

    InitGraphics();
    InitPipeline();

    // Set game list
    m_romList.push_back("");
    m_romList.push_back("armor_attack");
    m_romList.push_back("bedlam");
    m_romList.push_back("berzerk");
    m_romList.push_back("blitz");
    m_romList.push_back("clean_sweep");
    m_romList.push_back("cosmic_chasm");
    m_romList.push_back("fortress_of_narzord");
    m_romList.push_back("headsup");
    m_romList.push_back("hyperchase");
    m_romList.push_back("mine_storm");
    m_romList.push_back("polar_rescue");
    m_romList.push_back("pole_position");
    m_romList.push_back("rip-off");
    m_romList.push_back("scramble");
    m_romList.push_back("solar_quest");
    m_romList.push_back("space_wars");
    m_romList.push_back("spike");
    m_romList.push_back("spinball");
    m_romList.push_back("star_castle");
    m_romList.push_back("star_trek");
    m_romList.push_back("starhawk");
    m_romList.push_back("web_wars");

    LoadGame();
}

void CGame::RemapVertexBuffer() {
    D3D11_MAPPED_SUBRESOURCE mappedResource;
    ZeroMemory(&mappedResource, sizeof(D3D11_MAPPED_SUBRESOURCE));

    m_dxContext->Map(m_vertexBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
    memcpy(mappedResource.pData, m_vertices, m_verticeCount * sizeof(VERTEX));
    m_dxContext->Unmap(m_vertexBuffer.Get(), 0);
}

bool CGame::Update(InputController^ controller) {
    if (controller->IsViewButtonPressed()) {
        return true;
    }

    if (controller->IsMenuButtonPressed()) {
        NextGame();
    }

    if (controller->GetDirectionOfLeftStick() == InputControllerDirection::Left) {
        vectrex_emulator_key(PL2_LEFT, true);
    }
    if (controller->GetDirectionOfLeftStick() == InputControllerDirection::Right) {
        vectrex_emulator_key(PL2_RIGHT, true);
    }
    if (controller->GetDirectionOfLeftStick() == InputControllerDirection::Up) {
        vectrex_emulator_key(PL2_UP, true);
    }
    if (controller->GetDirectionOfLeftStick() == InputControllerDirection::Down) {
        vectrex_emulator_key(PL2_DOWN, true);
    }

    if (controller->GetDirectionOfRightStick() == InputControllerDirection::Left) {
        vectrex_emulator_key(PL1_LEFT, true);
    }
    if (controller->GetDirectionOfRightStick() == InputControllerDirection::Right) {
        vectrex_emulator_key(PL1_RIGHT, true);
    }
    if (controller->GetDirectionOfRightStick() == InputControllerDirection::Up) {
        vectrex_emulator_key(PL1_UP, true);
    }
    if (controller->GetDirectionOfRightStick() == InputControllerDirection::Down) {
        vectrex_emulator_key(PL1_DOWN, true);
    }

    if (controller->IsLeftTriggerPressed() || controller->IsRightTriggerPressed()) {
        vectrex_emulator_key(PL1_DOWN, true);
    }

    if (controller->IsXButtonPressed()) {
        vectrex_emulator_key(PL1_LEFT, true);
    }
    if (controller->IsYButtonPressed()) {
        vectrex_emulator_key(PL1_RIGHT, true);
    }
    if (controller->IsAButtonPressed()) {
        vectrex_emulator_key(PL1_UP, true);
    }
    if (controller->IsBButtonPressed()) {
        vectrex_emulator_key(PL1_DOWN, true);
    }

    m_verticeCount = 0;
    vectrex_emulator_frame();
    RemapVertexBuffer();

    vectrex_emulator_key(PL2_LEFT, false);
    vectrex_emulator_key(PL2_RIGHT, false);
    vectrex_emulator_key(PL2_UP, false);
    vectrex_emulator_key(PL2_DOWN, false);
    vectrex_emulator_key(PL1_LEFT, false);
    vectrex_emulator_key(PL1_RIGHT, false);
    vectrex_emulator_key(PL1_UP, false);
    vectrex_emulator_key(PL1_DOWN, false);

    return false;
}

void CGame::Render() {   
    m_dxContext->OMSetRenderTargets(1, m_dxRenderTarget.GetAddressOf(), nullptr);

    float color[4] = {0.0f, 0.0f, 0.0f, 1.0f};
    m_dxContext->ClearRenderTargetView(m_dxRenderTarget.Get(), color);
    
    UINT stride = sizeof(VERTEX);
    UINT offset = 0;
    m_dxContext->IASetVertexBuffers(0, 1, m_vertexBuffer.GetAddressOf(), &stride, &offset);

    m_dxContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP);

    for (int i = 0; i < m_verticeCount /2; i=i+2) {
        m_dxContext->Draw(2, i);
    }

    m_dxSwapChain->Present(1, 0);
}

void CGame::InitGraphics() {
    D3D11_BUFFER_DESC bd = { 0 };
    bd.ByteWidth = sizeof(VERTEX) * 2048;
    bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    bd.Usage = D3D11_USAGE_DYNAMIC;
    bd.MiscFlags = 0;
    bd.StructureByteStride = 0;

    D3D11_SUBRESOURCE_DATA srd = { m_vertices, 0, 0 };

    m_dxDevice->CreateBuffer(&bd, &srd, &m_vertexBuffer);
}

void CGame::InitPipeline() {
    Array<byte>^ vsFile = LoadShaderFile("VertexShader.cso");
    Array<byte>^ psFile = LoadShaderFile("PixelShader.cso");

    m_dxDevice->CreateVertexShader(vsFile->Data, vsFile->Length, nullptr, &m_vertexShader);
    m_dxDevice->CreatePixelShader(psFile->Data, psFile->Length, nullptr, &m_pixelShader);

    m_dxContext->VSSetShader(m_vertexShader.Get(), nullptr, 0);
    m_dxContext->PSSetShader(m_pixelShader.Get(), nullptr, 0);

    D3D11_INPUT_ELEMENT_DESC ied[] = {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
    };

    // create and set the input layout
    m_dxDevice->CreateInputLayout(ied, ARRAYSIZE(ied), vsFile->Data, vsFile->Length, &m_inputLayout);
    m_dxContext->IASetInputLayout(m_inputLayout.Get());
}

void CGame::AddLine(int x1, int y1, int x2, int y2, uint8_t color) {
    if (m_verticeCount >= 2048) {
        return;
    }

    float x = 1.0 - 1.0f / 310.0f * (float)x1 + 0.5;
    float y = 1.0 - 1.0f / 410.0f * (float)y1 - 0.25;
    m_vertices[m_verticeCount++] = { -1*x, y, 0.0f };

    x = 1.0-1.0f / 310 * (float)x2 + 0.5;
    y = 1.0-1.0f / 410.0f * (float)y2 - 0.25;
    m_vertices[m_verticeCount++] = { -1*x, y, 0.0f };
}
