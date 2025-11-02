#define INTEL_UHD_770_I7_13700HX_BENCH_GPX 11.967264
#define NOMINMAX
#include <iostream>
#include <Windows.h>
#include <chrono>
#include <algorithm>
#include <dxgi.h>
#include <dxgi1_2.h>
#include <d3d11.h>
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
void* screen_mem = malloc(width * height * sizeof(unsigned int));
void* screen_z = malloc(width * height * sizeof(uint16_t));
void* screen_a = malloc(sizeof(unsigned int) * width * height);
unsigned int triangle_count_1 = 18000;
unsigned int triangle_count_2 = 0;
unsigned int* retrieve;
ID3D11Device* device = nullptr;
ID3D11DeviceContext* ctx = nullptr;
D3D_FEATURE_LEVEL level;
ID3D11ComputeShader* compute_shader = nullptr;
ID3D11ComputeShader* z_shader = nullptr;
ID3D11ComputeShader* blend_shader = nullptr;
WNDCLASS w_class = { 0 };
ID3D11Buffer* buf1 = nullptr;
ID3D11Buffer* buf2 = nullptr;
ID3D11Buffer* buf3 = nullptr;
ID3D11Buffer* buf4 = nullptr;
ID3D11Buffer* buf5 = nullptr;
ID3D11Buffer* buf6 = nullptr;
ID3D11Buffer* buf7 = nullptr;
ID3D11Buffer* buf8 = nullptr;
ID3D11Buffer* CPU_buf = nullptr;
ID3D11UnorderedAccessView* UAV1 = nullptr;
ID3D11UnorderedAccessView* UAV2 = nullptr;
ID3D11ShaderResourceView* SRV1 = nullptr;
ID3D11ShaderResourceView* SRV2 = nullptr;
ID3D11ShaderResourceView* SRV3 = nullptr;
ID3D11ShaderResourceView* SRV4 = nullptr;
ID3D11ShaderResourceView* SRV5 = nullptr;
ID3D11ShaderResourceView* SRV6 = nullptr;
HRESULT hr;
Shader shader;
std::vector<unsigned int> misc;


LRESULT w_proc(HWND window, UINT msg, WPARAM wparam, LPARAM lparam) {
    switch (msg) {
    case WM_QUIT: {
        ctx->Flush();
        ctx->ClearState();
        ctx->Flush();

        running = false;

        if (compute_shader) compute_shader->Release();
        if (z_shader) z_shader->Release();
        if (blend_shader) blend_shader->Release();
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
        if (screen_mem) free(screen_mem);
        if (screen_z) free(screen_z);
        if (screen_a) free(screen_a);
        break;
    }
    case WM_DESTROY: {
        PostQuitMessage(0);
        return 0;
        break;
    } case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(window, &ps);

        if (screen_mem) {
            BITMAPINFO bmi = {};
            bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
            bmi.bmiHeader.biWidth = width;
            bmi.bmiHeader.biHeight = -height;
            bmi.bmiHeader.biPlanes = 1;
            bmi.bmiHeader.biBitCount = 32;
            bmi.bmiHeader.biCompression = BI_RGB;
            bmi.bmiHeader.biSizeImage = width * height * 4;

            StretchDIBits(hdc, 0, 0, width, height, 0, 0, width, height, retrieve, &bmi, DIB_RGB_COLORS, SRCCOPY);
        }

        EndPaint(window, &ps);
        break;
    } default: {
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

    HWND wnd = CreateWindowExW(0, L"class", L"My game engine, with just the windows API", WS_OVERLAPPEDWINDOW, 0, 0, width, height, 0, 0, h_inst, 0);
    ShowWindow(wnd, n_cmd_show);

    for (unsigned int i = 0; i < width * height; i++) {
        ((uint16_t*)screen_z)[i] = XMConvertFloatToHalf(FLT_MAX); // Start with maximum depth (farthest)
    }

    std::vector<float> spositions = spcopy();
    std::vector<float> scolors = sccopy();
    std::vector<float> sogp = spcopy();
    std::vector<float> sogc = sccopy();

    std::vector<float> bpositions = bpcopy();
    std::vector<float> bcolors = bccopy();
    std::vector<float> bogp = bpcopy();
    std::vector<float> bogc = bccopy();

    std::vector<float> tri_meta;
    misc = { width, height, triangle_count_1, 256, 256 }; // Screen width, screen height, triangle count
    std::vector<unsigned int> tex_vector = read_bmp("Untitled.bmp"); // ONLY 32-BIT MS-PAINT BMP TEXTURES SUPPORTED!
    std::vector<float> uniforms = { 0.1 };

    hr = D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0, nullptr, 0, D3D11_SDK_VERSION, &device, &level, &ctx);
    if (FAILED(hr)) { return -1; }
    shader.Shader_device(device, ctx, level);

    //
    //
    //

    hr = shader.create_D3D11_buffer(&buf1, (const void*)screen_mem, sizeof(unsigned int) * width * height, sizeof(unsigned int), D3D11_BIND_UNORDERED_ACCESS, 0, 0, D3D11_USAGE_DEFAULT);
    if (FAILED(hr)) { return 3; }

    hr = shader.create_D3D11_buffer(&buf2, (const void*)screen_z, sizeof(uint16_t) * width * height, sizeof(uint16_t), D3D11_BIND_UNORDERED_ACCESS, 0, 0, D3D11_USAGE_DEFAULT);
    if (FAILED(hr)) { return 4; }

    hr = shader.create_D3D11_buffer(&buf3, (const void*)spositions.data(), sizeof(float) * 9 * triangle_count_1, sizeof(float), D3D11_BIND_SHADER_RESOURCE, D3D11_RESOURCE_MISC_BUFFER_STRUCTURED, D3D11_CPU_ACCESS_WRITE, D3D11_USAGE_DYNAMIC);
    if (FAILED(hr)) { return 5; }

    hr = shader.create_D3D11_buffer(&buf4, (const void*)scolors.data(), sizeof(float) * 13 * triangle_count_1, sizeof(float), D3D11_BIND_SHADER_RESOURCE, D3D11_RESOURCE_MISC_BUFFER_STRUCTURED, D3D11_CPU_ACCESS_WRITE, D3D11_USAGE_DYNAMIC);
    if (FAILED(hr)) { return 6; }

    hr = shader.create_D3D11_buffer(&buf5, (const void*)misc.data(), sizeof(unsigned int) * 5, sizeof(unsigned int), D3D11_BIND_SHADER_RESOURCE, D3D11_RESOURCE_MISC_BUFFER_STRUCTURED, D3D11_CPU_ACCESS_WRITE, D3D11_USAGE_DYNAMIC);
    if (FAILED(hr)) { return 7; }

    hr = shader.create_D3D11_buffer(&buf6, (const void*)tri_meta.data(), sizeof(float) * 5 * triangle_count_1, sizeof(float), D3D11_BIND_SHADER_RESOURCE, D3D11_RESOURCE_MISC_BUFFER_STRUCTURED, D3D11_CPU_ACCESS_WRITE, D3D11_USAGE_DYNAMIC);
    if (FAILED(hr)) { return 7; }

    hr = shader.create_D3D11_buffer(&buf7, (const void*)uniforms.data(), sizeof(float) * uniforms.size() * triangle_count_1, sizeof(float), D3D11_BIND_SHADER_RESOURCE, D3D11_RESOURCE_MISC_BUFFER_STRUCTURED, D3D11_CPU_ACCESS_WRITE, D3D11_USAGE_DYNAMIC);
    if (FAILED(hr)) { return 7; }

    hr = shader.create_D3D11_buffer(&buf8, (const void*)tex_vector.data(), sizeof(unsigned int) * tex_vector.size(), sizeof(unsigned int), D3D11_BIND_SHADER_RESOURCE, D3D11_RESOURCE_MISC_BUFFER_STRUCTURED, D3D11_CPU_ACCESS_WRITE, D3D11_USAGE_DYNAMIC);
    if (FAILED(hr)) { return 7; }

    shader.compile_shader(L"solidrasterizer.hlsl", "main");
    hr = device->CreateComputeShader(shader.get_shader_blob()->GetBufferPointer(), shader.get_shader_blob()->GetBufferSize(), nullptr, &compute_shader);

    shader.compile_shader(L"blendedrasterizer.hlsl", "main");
    hr = device->CreateComputeShader(shader.get_shader_blob()->GetBufferPointer(), shader.get_shader_blob()->GetBufferSize(), nullptr, &blend_shader);

    shader.compile_shader(L"z.hlsl", "main");
    hr = device->CreateComputeShader(shader.get_shader_blob()->GetBufferPointer(), shader.get_shader_blob()->GetBufferSize(), nullptr, &z_shader);

    if (FAILED(hr)) { return hr; }

    hr = shader.create_UAV(buf1, &UAV1, width * height, DXGI_FORMAT_R32_UINT);
    if (FAILED(hr)) { std::cout << "Error creating D3D11 UAV\n" << std::hex << hr << "\n"; return -4; }

    hr = shader.create_UAV(buf2, &UAV2, width * height, DXGI_FORMAT_R16_FLOAT);
    if (FAILED(hr)) { std::cout << "Error creating D3D11 UAV\n" << std::hex << hr << "\n"; return -4; }

    ID3D11UnorderedAccessView* UAVs[] = { UAV1, UAV2 };

    hr = shader.create_SRV(buf3, &SRV1, 9 * triangle_count_1, D3D11_SRV_DIMENSION_BUFFER);
    if (FAILED(hr)) { std::cout << "Error creating D3D11 SRV\n" << std::hex << hr << "\n"; return -5; }

    hr = shader.create_SRV(buf4, &SRV2, 13 * triangle_count_1, D3D11_SRV_DIMENSION_BUFFER);
    if (FAILED(hr)) { std::cout << "Error creating D3D11 SRV\n" << std::hex << hr << "\n"; return -5; }

    hr = shader.create_SRV(buf5, &SRV3, 5, D3D11_SRV_DIMENSION_BUFFER);
    if (FAILED(hr)) { std::cout << "Error creating D3D11 SRV\n" << std::hex << hr << "\n"; return -5; }

    hr = shader.create_SRV(buf6, &SRV4, 5 * triangle_count_1, D3D11_SRV_DIMENSION_BUFFER);
    if (FAILED(hr)) { std::cout << "Error creating D3D11 SRV\n" << std::hex << hr << "\n"; return -5; }

    hr = shader.create_SRV(buf7, &SRV5, uniforms.size(), D3D11_SRV_DIMENSION_BUFFER);
    if (FAILED(hr)) { std::cout << "Error creating D3D11 SRV\n" << std::hex << hr << "\n"; return -5; }

    hr = shader.create_SRV(buf8, &SRV6, tex_vector.size(), D3D11_SRV_DIMENSION_BUFFER);
    if (FAILED(hr)) { std::cout << "Error creating D3D11 SRV\n" << std::hex << hr << "\n"; return -5; }

    ID3D11ShaderResourceView* SRVs[] = { SRV1, SRV2, SRV3, SRV4, SRV5, SRV6 };

    hr = shader.create_D3D11_buffer(&CPU_buf, nullptr, sizeof(unsigned int) * width * height, sizeof(unsigned int), 0, 0, D3D11_CPU_ACCESS_READ, D3D11_USAGE_STAGING);
    if (FAILED(hr)) { std::cout << "Error creating D3D11 CPU buffer\n"; return -5; }

    D3D11_MAPPED_SUBRESOURCE mapped;

    //
    //
    //

    ID3D11UnorderedAccessView* nullUAV = { nullptr };
    ID3D11ShaderResourceView* nullSRV = { nullptr };

    MSG msg = {};
    float thetax = 0;
    float thetay = 0;
    bool right = true;
    while (running && device && ctx) {
        // Check Windows messages
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        tri_meta.clear();
        tri_meta.reserve(triangle_count_1 * 5);

        ctx->CSSetShader(z_shader, nullptr, 0); // Bind the compute shader

        ctx->CSSetUnorderedAccessViews(0, 2, UAVs, nullptr);
        ctx->Dispatch(ceil((1920 * 1080) / 1024), 1, 1);

        auto start = std::chrono::high_resolution_clock::now();

        for (unsigned int i = 0; i < triangle_count_1 * 9; i += 3) {
            float x = sogp[i];
            float y = sogp[i + 1];
            float z = sogp[i + 2];

            float ry = radians(thetax);
            float rx = radians(thetay);

            float y1 = y * cos(rx) - z * sin(rx);
            float z1 = y * sin(rx) + z * cos(rx);
            float x1 = x;

            float xr = x1 * cos(ry) + z1 * sin(ry);
            float yr = y1;
            float zr = -x1 * sin(ry) + z1 * cos(ry);

            if (((thetax > 70) && (thetax < 290)) || ((thetay > 70) && (thetay < 290))) {
                // Don’t push this vertex at all
                spositions[i] = -1;
                spositions[i + 1] = -1;
                spositions[i + 2] = -1;
                continue;
            }

            float invz = 1.0f / zr;
            float xp = (xr * 0.5f + 0.5f) * width;
            float yp = (yr * -0.5f + 0.5f) * height;

            spositions[i] = xp;
            spositions[i + 1] = yp;
            spositions[i + 2] = zr;
        }

        for (unsigned int i = 0; i < triangle_count_1; i++) {
            int minx = std::min(spositions[i * 9], std::min(spositions[i * 9 + 3], spositions[i * 9 + 6]));
            int miny = std::min(spositions[i * 9 + 1], std::min(spositions[i * 9 + 4], spositions[i * 9 + 7]));
            int maxx = std::max(spositions[i * 9], std::max(spositions[i * 9 + 3], spositions[i * 9 + 6]));
            int maxy = std::max(spositions[i * 9 + 1], std::max(spositions[i * 9 + 4], spositions[i * 9 + 7]));

            float inv_area = 1.0f / ((spositions[i * 9] - spositions[i * 9 + 3]) * (spositions[i * 9 + 7] - spositions[i * 9 + 4]) - (spositions[i * 9 + 1] - spositions[i * 9 + 4]) * (spositions[i * 9 + 6] - spositions[i * 9 + 3]));

            tri_meta.push_back((float)minx);
            tri_meta.push_back((float)miny);
            tri_meta.push_back((float)maxx);
            tri_meta.push_back((float)maxy);
            tri_meta.push_back(inv_area);
        }

        ctx->CSSetShader(compute_shader, nullptr, 0); // Bind the compute shader
        ctx->CSSetUnorderedAccessViews(0, 1, &nullUAV, nullptr); // Bind UAV to slot u0
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

        ctx->CSSetShader(compute_shader, nullptr, 0); // Bind the compute shader
        ctx->CSSetUnorderedAccessViews(0, 2, UAVs, nullptr); // Bind UAV to slot u0
        ctx->CSSetShaderResources(0, 6, SRVs);
        ctx->Dispatch(triangle_count_1, 1, 1);

        retrieve = shader.retrieve_D3D11_process<unsigned int>(CPU_buf, buf1);
        auto end = std::chrono::high_resolution_clock::now();

        //
        //
        // SECOND DRAW CALL
        //
        //

        tri_meta.clear();
        tri_meta.reserve(triangle_count_2 * 5);

        for (unsigned int i = 0; i < triangle_count_2 * 9; i += 3) {
            float x = bogp[i];
            float y = bogp[i + 1];
            float z = bogp[i + 2];

            float ry = radians(thetax);
            float rx = radians(thetay);

            float y1 = y * cos(rx) - z * sin(rx);
            float z1 = y * sin(rx) + z * cos(rx);
            float x1 = x;

            float xr = x1 * cos(ry) + z1 * sin(ry);
            float yr = y1;
            float zr = -x1 * sin(ry) + z1 * cos(ry);

            if (((thetax > 70) && (thetax < 290)) || ((thetay > 70) && (thetay < 290))) {
                // Don’t push this vertex at all
                bpositions[i] = -1;
                bpositions[i + 1] = -1;
                bpositions[i + 2] = -1;
                continue;
            }

            float invz = 1.0f / zr;
            float xp = (xr * 0.5f + 0.5f) * width;
            float yp = (yr * -0.5f + 0.5f) * height;

            bpositions[i] = xp;
            bpositions[i + 1] = yp;
            bpositions[i + 2] = zr;
        }

        for (unsigned int i = 0; i < triangle_count_2; i++) {
            int minx = std::min(bpositions[i * 9], std::min(bpositions[i * 9 + 3], bpositions[i * 9 + 6]));
            int miny = std::min(bpositions[i * 9 + 1], std::min(bpositions[i * 9 + 4], bpositions[i * 9 + 7]));
            int maxx = std::max(bpositions[i * 9], std::max(bpositions[i * 9 + 3], bpositions[i * 9 + 6]));
            int maxy = std::max(bpositions[i * 9 + 1], std::max(bpositions[i * 9 + 4], bpositions[i * 9 + 7]));

            float inv_area = 1.0f / ((bpositions[i * 9] - bpositions[i * 9 + 3]) * (bpositions[i * 9 + 7] - bpositions[i * 9 + 4]) - (bpositions[i * 9 + 1] - bpositions[i * 9 + 4]) * (bpositions[i * 9 + 6] - bpositions[i * 9 + 3]));

            tri_meta.push_back((float)minx);
            tri_meta.push_back((float)miny);
            tri_meta.push_back((float)maxx);
            tri_meta.push_back((float)maxy);
            tri_meta.push_back(inv_area);
        }

        ctx->CSSetShader(blend_shader, nullptr, 0); // Bind the compute shader
        ctx->CSSetUnorderedAccessViews(0, 1, &nullUAV, nullptr); // Bind UAV to slot u0
        ctx->CSSetShaderResources(0, 1, &nullSRV);

        ctx->Map(buf6, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
        memcpy(mapped.pData, tri_meta.data(), sizeof(float) * tri_meta.size());
        ctx->Unmap(buf6, 0);

        ctx->Map(buf3, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
        memcpy(mapped.pData, bpositions.data(), sizeof(float) * triangle_count_2 * 9);
        ctx->Unmap(buf3, 0);

        ctx->Map(buf4, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
        memcpy(mapped.pData, bcolors.data(), sizeof(float) * triangle_count_2 * 13);
        ctx->Unmap(buf4, 0);

        ctx->CSSetShader(blend_shader, nullptr, 0); // Bind the compute shader
        ctx->CSSetUnorderedAccessViews(0, 2, UAVs, nullptr); // Bind UAV to slot u0
        ctx->CSSetShaderResources(0, 6, SRVs);
        ctx->Dispatch(triangle_count_2, 1, 1);

        retrieve = shader.retrieve_D3D11_process<unsigned int>(CPU_buf, buf1);

        InvalidateRect(wnd, nullptr, true);

        std::chrono::duration<double, std::milli> elapsed = end - start;
        std::cout << "FPS: " << 1000 / elapsed.count() << " fps\n";
        //thetax += 0.05;
        //thetay += 0.05;

        //thetax = std::fmodf(thetax, 360.0f);
        //thetay = std::fmodf(thetay, 360.0f);
    }

    return 0;
}
