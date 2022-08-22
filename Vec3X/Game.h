#pragma once

#include "InputController.h"
#include <string>
#include <vector>

using namespace Microsoft::WRL;
using namespace Windows::UI::Core;
using namespace Platform;
using namespace DirectX;

// a struct to represent a single vertex
struct VERTEX {
    float x, y, z;
};

class CGame {
public:
    void Initialize();
    bool Update(InputController^ controller);
    void Render();

    void AddLine(int x1, int y1, int x2, int y2, uint8_t color);

private:
    void RemapVertexBuffer();
    void InitGraphics();
    void InitPipeline();
    void LoadGame();
    void NextGame();

private:
    ComPtr<ID3D11Device1> m_dxDevice;                   // the device interface
    ComPtr<ID3D11DeviceContext1> m_dxContext;           // the device context interface
    ComPtr<IDXGISwapChain1> m_dxSwapChain;              // the swap chain interface
    ComPtr<ID3D11RenderTargetView> m_dxRenderTarget;    // the render target interface
    ComPtr<ID3D11Buffer> m_vertexBuffer;                // the vertex buffer interface
    ComPtr<ID3D11VertexShader> m_vertexShader;          // the vertex shader interface
    ComPtr<ID3D11PixelShader> m_pixelShader;            // the pixel shader interface
    ComPtr<ID3D11InputLayout> m_inputLayout;            // the input layout interface
 
    VERTEX m_vertices[2048] = { };
    int m_verticeCount = 0;

    std::vector<std::string> m_romList;
    int m_selectedRom = 0;
};
