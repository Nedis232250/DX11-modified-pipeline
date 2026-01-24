#define INTEL_UHD_770_I7_13700HX_BENCH_GPX_NO_Z_CULL 7.768224
#define NVIDIA_GEFORCE_RTX_4060_LAPTOP_GPU_I7_13700HX_BENCH_GPX_NO_Z_CULL 59.836320
#define NOMINMAX
#include <iostream>
#include <Windows.h>
#include <chrono>
#include <algorithm>
#include <dxgi.h>
#include <dxgi1_2.h>
#include <d3d11.h>
#include <d3d11_1.h>
#include <vector>
#include <thread>
#include <array>
#include <DirectXMath.h>
#include <DirectXPackedVector.h>

#include "v.h"
#include "utils.hpp"
#pragma comment(linker, "/SUBSYSTEM:WINDOWS")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "d3dcompiler.lib")

using namespace DirectX;
using namespace DirectX::PackedVector;

/*

IMPORTANT NOTES:

TRIANGLES MUST HAVE 2 DRAW CALLS. 1 FOR THE SOLID TRIANGLES (first call) AND ONE FOR THE TRANSPARENT/TRANSLUCENT ONES (second call) OR OTHERWISE BLENDING IS FUCKED.

*/

bool running = true;
unsigned short width = 1920;
unsigned short height = 1080;
unsigned int triangle_count_1 = 18000;
std::vector<unsigned int> retrieve1(width * height);
std::vector<unsigned int> retrieve2(width * height);
float thetax = 0;
float thetay = 0;

HRESULT hr;
ID3D11Device* device = nullptr;
ID3D11DeviceContext* ctx = nullptr;
ID3D11ComputeShader* compute_shader = nullptr;
ID3D11ComputeShader* z_shader = nullptr;
ID3D11ComputeShader* _540p_mip_shader = nullptr;
ID3D11ComputeShader* mipclear = nullptr;
ID3D11Texture2D* buf1 = nullptr;
ID3D11Texture2D* buf2 = nullptr;
ID3D11Buffer* buf3 = nullptr;
ID3D11Buffer* buf4 = nullptr;
ID3D11Buffer* buf5 = nullptr;
ID3D11Buffer* buf6 = nullptr;
ID3D11Buffer* buf7 = nullptr;
ID3D11Buffer* buf8 = nullptr;
ID3D11Texture2D* CPU_buf = nullptr;
ID3D11UnorderedAccessView* UAV1 = nullptr;
ID3D11UnorderedAccessView* UAV2 = nullptr;
ID3D11ShaderResourceView* SRV1 = nullptr;
ID3D11ShaderResourceView* SRV2 = nullptr;
ID3D11ShaderResourceView* SRV3 = nullptr;
ID3D11ShaderResourceView* SRV4 = nullptr;
ID3D11ShaderResourceView* SRV5 = nullptr;
ID3D11ShaderResourceView* SRV6 = nullptr;
ID3D11Texture2D* _540p_buf1 = nullptr;

HWND wnd;
PAINTSTRUCT ps_;
BITMAPINFO bmi = {};
WNDCLASSW w_class = { 0 };

Shader shader;
mip_resources mip_resources_;
std::vector<Triangle> triangles;
std::vector<unsigned int> misc = { width, height, triangle_count_1, 256, 256 };
std::vector<float> spositions = spcopy();
std::vector<float> scolors = sccopy();
std::vector<float> sogp = spcopy();
std::vector<float> sogc = sccopy();
std::vector<float> tri_meta;
std::vector<float> _540p_tri_meta;
std::vector<float> _540p_spositions = spcopy();
std::vector<int> passed_vector(triangle_count_1);

LRESULT w_proc(HWND window, UINT msg, WPARAM wparam, LPARAM lparam) {
    switch (msg) {
    case WM_QUIT: {
        ctx->Flush();
        ctx->ClearState();
        ctx->Flush();

        running = false;

        if (compute_shader) compute_shader->Release();
        if (z_shader) z_shader->Release();
        if (ctx) ctx->Release();
        if (device) device->Release();
        if (CPU_buf) CPU_buf->Release();
        if (buf1) buf1->Release();
        if (buf2) buf2->Release();
        if (buf3) buf3->Release();
        if (buf4) buf4->Release();
        if (buf5) buf5->Release();
        if (buf6) buf6->Release();
        if (buf7) buf7->Release();
        if (buf8) buf8->Release();
        if (UAV1) UAV1->Release();
        if (UAV2) UAV2->Release();
        if (SRV1) SRV1->Release();
        if (SRV2) SRV2->Release();
        if (SRV3) SRV3->Release();
        if (SRV4) SRV4->Release();
        if (SRV5) SRV5->Release();
        if (SRV6) SRV6->Release();
        if (_540p_mip_shader) _540p_mip_shader->Release();
        if (_540p_buf1) _540p_buf1->Release();

        break;
    }
    case WM_DESTROY: {
        ctx->Flush();
        ctx->ClearState();
        ctx->Flush();

        running = false;

        if (compute_shader) compute_shader->Release();
        if (z_shader) z_shader->Release();
        if (ctx) ctx->Release();
        if (device) device->Release();
        if (CPU_buf) CPU_buf->Release();
        if (buf1) buf1->Release();
        if (buf2) buf2->Release();
        if (buf3) buf3->Release();
        if (buf4) buf4->Release();
        if (buf5) buf5->Release();
        if (buf6) buf6->Release();
        if (buf7) buf7->Release();
        if (buf8) buf8->Release();
        if (UAV1) UAV1->Release();
        if (UAV2) UAV2->Release();
        if (SRV1) SRV1->Release();
        if (SRV2) SRV2->Release();
        if (SRV3) SRV3->Release();
        if (SRV4) SRV4->Release();
        if (SRV5) SRV5->Release();
        if (SRV6) SRV6->Release();
        if (_540p_mip_shader) _540p_mip_shader->Release();
        if (_540p_buf1) _540p_buf1->Release();

        break;
    }
    case WM_PAINT: {
        HDC hdc = BeginPaint(window, &ps_);
        bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        bmi.bmiHeader.biWidth = width;
        bmi.bmiHeader.biHeight = -(int)height;
        bmi.bmiHeader.biPlanes = 1;
        bmi.bmiHeader.biBitCount = 32;
        bmi.bmiHeader.biCompression = BI_RGB;
        bmi.bmiHeader.biSizeImage = width * height * 4;

        StretchDIBits(hdc, 0, 0, width, height, 0, 0, width, height, retrieve1.data(), &bmi, DIB_RGB_COLORS, SRCCOPY);

        EndPaint(window, &ps_);
        break;
    }
    default: {
        return DefWindowProc(window, msg, wparam, lparam);
    }
    }
}

int WinMain(_In_ HINSTANCE h_inst, _In_opt_ HINSTANCE p_inst, _In_ LPSTR lpcmdln, _In_ int n_cmd_show) {
    attach_console();
    SetProcessDPIAware();
    
    w_class.lpszClassName = L"class";
    w_class.lpfnWndProc = w_proc;
    w_class.hInstance = h_inst;
    w_class.style = CS_HREDRAW | CS_VREDRAW;

    RegisterClassW(&w_class);

    wnd = CreateWindowExW(0, L"class", L"My game engine, with just the windows API", WS_OVERLAPPEDWINDOW, 0, 0, width, height, 0, 0, h_inst, 0);
    ShowWindow(wnd, n_cmd_show);
    
    std::vector<unsigned int> tex_vector = read_bmp("Untitled.bmp"); // ONLY 32-BIT MS-PAINT BMP TEXTURES SUPPORTED!
    
    hr = D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0, nullptr, 0, D3D11_SDK_VERSION, &device, nullptr, &ctx);
    if (FAILED(hr)) { return -1; }
    shader.Shader_device(device, ctx);
    
    //
    //
    //

    D3D11_TEXTURE2D_DESC desc = {};
    desc.Width = width;                  // must be >0
    desc.Height = height;                // must be >0
    desc.MipLevels = 1;                  // you only need base levelm
    desc.ArraySize = 1;                  // 1 texture in array
    desc.Format = DXGI_FORMAT_R32_UINT; // 32-bit float for depth
    desc.SampleDesc.Count = 1;           // no MSAA
    desc.SampleDesc.Quality = 0;
    desc.Usage = D3D11_USAGE_DEFAULT;    // GPU read/write
    desc.BindFlags = D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_SHADER_RESOURCE;
    desc.CPUAccessFlags = 0;             // CPU won't read/write
    desc.MiscFlags = 0;                  // nothing special

    hr = device->CreateTexture2D(&desc, nullptr, &buf1);
    if (FAILED(hr)) { return -2; }

    desc.Width = width;                  // must be >0
    desc.Height = height;                // must be >0
    desc.MipLevels = 1;                  // you only need base level
    desc.ArraySize = 1;                  // 1 texture in array
    desc.Format = DXGI_FORMAT_R32_UINT;  // 32-bit float for depth
    desc.SampleDesc.Count = 1;           // no MSAA
    desc.SampleDesc.Quality = 0;
    desc.Usage = D3D11_USAGE_DEFAULT;    // GPU read/write
    desc.BindFlags = D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_SHADER_RESOURCE;
    desc.CPUAccessFlags = 0;             // CPU won't read/write
    desc.MiscFlags = 0;                  // nothing special

    hr = device->CreateTexture2D(&desc, nullptr, &buf2);
    if (FAILED(hr)) return -3;

    desc.Width = width;                  // must be >0
    desc.Height = height;                // must be >0
    desc.MipLevels = 1;                  // you only need base level
    desc.ArraySize = 1;                  // 1 texture in array
    desc.Format = DXGI_FORMAT_R32_UINT;  // 32-bit float for depth
    desc.SampleDesc.Count = 1;           // no MSAA
    desc.SampleDesc.Quality = 0;
    desc.Usage = D3D11_USAGE_STAGING;    // GPU read/write
    desc.BindFlags = 0;
    desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;             // CPU won't read/write
    desc.MiscFlags = 0;                  // nothing special

    hr = device->CreateTexture2D(&desc, nullptr, &CPU_buf);
    if (FAILED(hr)) { return -19; }

    //
    //
    //

    hr = shader.create_D3D11_buffer(&buf3, (const void*)spositions.data(), sizeof(float) * 9 * triangle_count_1, sizeof(float), D3D11_BIND_SHADER_RESOURCE, D3D11_RESOURCE_MISC_BUFFER_STRUCTURED, D3D11_CPU_ACCESS_WRITE, D3D11_USAGE_DYNAMIC);
    if (FAILED(hr)) { return -4; }

    hr = shader.create_D3D11_buffer(&buf4, (const void*)scolors.data(), sizeof(float) * 13 * triangle_count_1, sizeof(float), D3D11_BIND_SHADER_RESOURCE, D3D11_RESOURCE_MISC_BUFFER_STRUCTURED, D3D11_CPU_ACCESS_WRITE, D3D11_USAGE_DYNAMIC);
    if (FAILED(hr)) { return -5; }

    hr = shader.create_D3D11_buffer(&buf5, (const void*)misc.data(), sizeof(unsigned int) * 5, sizeof(unsigned int), D3D11_BIND_SHADER_RESOURCE, D3D11_RESOURCE_MISC_BUFFER_STRUCTURED, D3D11_CPU_ACCESS_WRITE, D3D11_USAGE_DYNAMIC);
    if (FAILED(hr)) { return -6; }

    hr = shader.create_D3D11_buffer(&buf6, (const void*)tri_meta.data(), sizeof(float) * 13 * triangle_count_1, sizeof(float), D3D11_BIND_SHADER_RESOURCE, D3D11_RESOURCE_MISC_BUFFER_STRUCTURED, D3D11_CPU_ACCESS_WRITE, D3D11_USAGE_DYNAMIC);
    if (FAILED(hr)) { return -7; }

    hr = shader.create_D3D11_buffer(&buf8, (const void*)tex_vector.data(), sizeof(float) * tex_vector.size(), sizeof(float), D3D11_BIND_SHADER_RESOURCE, D3D11_RESOURCE_MISC_BUFFER_STRUCTURED, D3D11_CPU_ACCESS_WRITE, D3D11_USAGE_DYNAMIC);
    if (FAILED(hr)) { return -8; }

    shader.compile_shader(L"solidrasterizer.hlsl", "main", "cs_5_0");
    hr = device->CreateComputeShader(shader.get_shader_blob()->GetBufferPointer(), shader.get_shader_blob()->GetBufferSize(), nullptr, &compute_shader);
    if (shader.get_shader_blob()) {
        shader.get_shader_blob()->Release();
    }

    if (shader.get_error_blob()) {
        shader.get_error_blob();
    }

    if (FAILED(hr)) return -9;

    shader.compile_shader(L"z.hlsl", "main", "cs_5_0");
    hr = device->CreateComputeShader(shader.get_shader_blob()->GetBufferPointer(), shader.get_shader_blob()->GetBufferSize(), nullptr, &z_shader);
    if (shader.get_shader_blob()) {
        shader.get_shader_blob()->Release();
    }

    if (shader.get_error_blob()) {
        shader.get_error_blob();
    }

    if (FAILED(hr)) return -10;

    hr = shader.create_UAV(buf1, &UAV1, width * height, DXGI_FORMAT_R32_UINT, 0, D3D11_UAV_DIMENSION_TEXTURE2D);
    if (FAILED(hr)) { std::cout << "Error creating D3D11 UAV\n" << std::hex << hr << "\n"; return -12; }

    hr = shader.create_UAV(buf2, &UAV2, width * height, DXGI_FORMAT_R32_UINT, 0, D3D11_UAV_DIMENSION_TEXTURE2D);
    if (FAILED(hr)) { std::cout << "Error creating D3D11 UAV\n" << std::hex << hr << "\n"; return -13; }

    hr = shader.create_SRV(buf3, &SRV1, 9 * triangle_count_1, DXGI_FORMAT_UNKNOWN, D3D11_SRV_DIMENSION_BUFFER);
    if (FAILED(hr)) { std::cout << "Error creating D3D11 SRV\n" << std::hex << hr << "\n"; return -14; }

    hr = shader.create_SRV(buf4, &SRV2, 13 * triangle_count_1, DXGI_FORMAT_UNKNOWN, D3D11_SRV_DIMENSION_BUFFER);
    if (FAILED(hr)) { std::cout << "Error creating D3D11 SRV\n" << std::hex << hr << "\n"; return -15; }

    hr = shader.create_SRV(buf5, &SRV3, 5, DXGI_FORMAT_UNKNOWN, D3D11_SRV_DIMENSION_BUFFER);
    if (FAILED(hr)) { std::cout << "Error creating D3D11 SRV\n" << std::hex << hr << "\n"; return -16; }

    hr = shader.create_SRV(buf6, &SRV4, 13 * triangle_count_1, DXGI_FORMAT_UNKNOWN, D3D11_SRV_DIMENSION_BUFFER);
    if (FAILED(hr)) { std::cout << "Error creating D3D11 SRV\n" << std::hex << hr << "\n"; return -17; }

    hr = shader.create_SRV(buf8, &SRV6, tex_vector.size(), DXGI_FORMAT_UNKNOWN, D3D11_SRV_DIMENSION_BUFFER);
    if (FAILED(hr)) { std::cout << "Error creating D3D11 SRV\n" << std::hex << hr << "\n"; return -18; }

    ID3D11UnorderedAccessView* nullUAV = { nullptr };
    ID3D11ShaderResourceView* nullSRV = { nullptr };
    ID3D11UnorderedAccessView* UAVs[] = { UAV1, UAV2 };
    ID3D11ShaderResourceView* SRVs[] = { SRV1, SRV2, SRV3, SRV4, SRV6 };

    //
    //
    //

    MSG msg = {};
    D3D11_MAPPED_SUBRESOURCE mapped;

    POINT cursor;
    GetCursorPos(&cursor);

    unsigned int startx = cursor.x;
    unsigned int starty = cursor.y;
    
    while (running && device && ctx) {
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        GetCursorPos(&cursor);

        tri_meta.clear();
        tri_meta.reserve(triangle_count_1 * 13);
        _540p_tri_meta.clear();
        _540p_tri_meta.reserve(triangle_count_1 * 13);
        triangles.clear();
        triangles.resize(triangle_count_1);

        CPU_vertex_transformation(sogp, spositions, triangle_count_1, thetax, thetay);

        for (unsigned int i = 0; i < sogc.size(); i++) {
            scolors[i] = sogc[i];
        }

        CPU_sort_shader(spositions, scolors, triangles, triangle_count_1);
        CPU_triangle_metadata(tri_meta, spositions, triangle_count_1, width, height);
        
        //
        //
        //

        ctx->CSSetUnorderedAccessViews(0, 1, &nullUAV, nullptr);
        ctx->CSSetShaderResources(0, 1, &nullSRV);

        ctx->Map(buf6, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
        memcpy(mapped.pData, tri_meta.data(), sizeof(float) * tri_meta.size());
        ctx->Unmap(buf6, 0);

        ctx->Map(buf3, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
        memcpy(mapped.pData, spositions.data(), sizeof(float) * triangle_count_1 * 9);
        ctx->Unmap(buf3, 0);

        ctx->Map(buf4, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
        memcpy(mapped.pData, scolors.data(), sizeof(float) * triangle_count_1 * 13);
        ctx->Unmap(buf4, 0);
        
        ctx->CSSetShader(z_shader, nullptr, 0); // Bind the compute shader
        ctx->CSSetUnorderedAccessViews(0, 2, UAVs, nullptr);
        ctx->CSSetShaderResources(0, 1, &SRV3);
        ctx->Dispatch(ceil((width * height) / 1024), 1, 1);

        auto start1 = std::chrono::high_resolution_clock::now();

        ctx->CSSetShader(compute_shader, nullptr, 0); // Bind the compute shader
        ctx->CSSetUnorderedAccessViews(0, 2, UAVs, nullptr); // Bind UAV to slot u0
        ctx->CSSetShaderResources(0, 5, SRVs);
        ctx->Dispatch(triangle_count_1, 1, 1);

        shader.retrieve_D3D11_process<unsigned int>(retrieve1.data(), CPU_buf, buf1, ctx, false, 0, 0, width * height * sizeof(unsigned int)); // software framebuffer
        
        auto end1 = std::chrono::high_resolution_clock::now();

        InvalidateRect(wnd, nullptr, true);

        std::chrono::duration<double, std::milli> elapsed = (end1 - start1);
        std::cout << "FPS: " << round(1000 / elapsed.count()) << " fps\n";
        //thetax = float((int)cursor.x - (int)startx) * -0.025;
        //thetay = float((int)cursor.y - (int)starty) * -0.025;
    }

    return 0;
}
