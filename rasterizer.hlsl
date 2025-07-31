#define NOMINMAX
#include <iostream>
#include <Windows.h>
#include <chrono>
#include <algorithm>
#include <thread>
#include <dxgi.h>
#include <dxgi1_2.h>
#include <d3d11.h>
#include <vector>
#include "v.h"
#include "utils.hpp"
#pragma comment(linker, "/SUBSYSTEM:WINDOWS")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "d3dcompiler.lib")

struct Fragment {
    float color1[4];
    float color2[4];
    float depth1;
    float depth2;
};

bool running = true;
unsigned short width = 1920;
unsigned short height = 1080;
void* screen_mem = malloc(width * height * sizeof(unsigned int));
void* screen_z = malloc(width * height * sizeof(float));
void* frag_buf = malloc(width * height * sizeof(Fragment));
unsigned int triangle_count = 1000;
unsigned int* retreive;
ID3D11Device* device = nullptr;
ID3D11DeviceContext* ctx = nullptr;
D3D_FEATURE_LEVEL level;
ID3D11ComputeShader* compute_shader;
WNDCLASS w_class = { 0 };
ID3D11Buffer* buf1;
ID3D11Buffer* buf2;
ID3D11Buffer* buf3;
ID3D11Buffer* CPU_buf;
HRESULT hr;
Shader shader;

struct RGB {
    unsigned int r, g, b;
};

LRESULT w_proc(HWND window, UINT msg, WPARAM wparam, LPARAM lparam) {
    switch (msg) {
    case WM_SIZE: {
        width = LOWORD(lparam);
        height = HIWORD(lparam);

        if (screen_mem) free(screen_mem);
        screen_mem = malloc(sizeof(unsigned int) * width * height);
        if (screen_mem == nullptr) {
            std::cerr << "Failed to allocate screen memory.";
            exit(EXIT_FAILURE); // Handle error gracefully
        }

        if (screen_z) free(screen_z);
        screen_z = malloc(sizeof(float) * width * height);
        if (screen_z == nullptr) {
            std::cerr << "Failed to allocate z memory.";
            exit(EXIT_FAILURE); // Handle error gracefully
        }

        if (frag_buf) free(frag_buf);
        frag_buf = malloc(sizeof(Fragment) * width * height);
        if (frag_buf == nullptr) {
            std::cerr << "Failed to allocate a memory.";
            exit(EXIT_FAILURE); // Handle error gracefully
        }

        if (device) {
            if (buf1) buf1->Release();
            if (buf2) buf2->Release();
            if (CPU_buf) CPU_buf->Release();

            hr = shader.create_D3D11_buffer(&buf1, (const void*)screen_mem, sizeof(unsigned int) * width * height, sizeof(unsigned int), D3D11_BIND_UNORDERED_ACCESS, 0, 0, D3D11_USAGE_DEFAULT);
            if (FAILED(hr)) { return 1; }

            hr = shader.create_D3D11_buffer(&buf2, (const void*)screen_z, sizeof(float) * width * height, sizeof(float), D3D11_BIND_UNORDERED_ACCESS, 0, 0, D3D11_USAGE_DEFAULT);
            if (FAILED(hr)) { return 2; }

            hr = shader.create_D3D11_buffer(&buf3, (const void*)frag_buf, sizeof(Fragment) * width * height, sizeof(Fragment), D3D11_BIND_UNORDERED_ACCESS, 0, 0, D3D11_USAGE_DEFAULT);
            if (FAILED(hr)) { return 3; }

            hr = shader.create_D3D11_buffer(&CPU_buf, nullptr, sizeof(float) * width * height, sizeof(float), 0, 0, D3D11_CPU_ACCESS_READ, D3D11_USAGE_STAGING);
            if (FAILED(hr)) { std::cout << "Error creating D3D11 CPU buffer\n"; return -5; }
        }

        break;
    }
    case WM_DESTROY: {
        if (screen_mem) free(screen_mem);
        if (screen_z) free(screen_z);
        running = false;
        PostQuitMessage(0);
        return 0;
    }
    case WM_PAINT: {
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

            StretchDIBits(hdc, 0, 0, width, height, 0, 0, width, height, screen_mem, &bmi, DIB_RGB_COLORS, SRCCOPY);
        }

        EndPaint(window, &ps);
        break;
    }
    default: {
        return DefWindowProc(window, msg, wparam, lparam);
    }
    }
}

int WinMain(HINSTANCE h_inst, HINSTANCE p_inst, LPSTR lpcmdln, int n_cmd_show) {
    attach_console();

    w_class.lpszClassName = L"class";
    w_class.lpfnWndProc = w_proc;
    w_class.hInstance = h_inst;

    RegisterClassW(&w_class);

    HWND wnd = CreateWindowExW(0, L"class", L"My game engine, with just the windows API", WS_OVERLAPPEDWINDOW, 0, 0, width, height, 0, 0, h_inst, 0);
    ShowWindow(wnd, n_cmd_show);

    for (int i = 0; i < width * height; i++) {
        ((float*)screen_z)[i] = FLT_MAX; // Start with maximum depth (farthest)
    }

    float* positions = pcopy();
    float* colors = ccopy();

    unsigned int misc[] = {
        width, height, triangle_count
    }; // Screen width, screen height, triangle count

    float uniform_[] = {
        1.0f
    }; // Stays constant size, used for things that change like camera angles, player positions, ETC.....

    std::vector<unsigned int> tex_vector = read_bmp("idiot.bmp"); // ONLY 24-BIT MS-PAINT BMP TEXTURES SUPPORTED!
    unsigned int* tex = tex_vector.data();

    std::vector<float> tri_meta;

    hr = D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0, nullptr, 0, D3D11_SDK_VERSION, &device, &level, &ctx);
    if (FAILED(hr)) { return -1; }
    shader.Shader_device(device, ctx, level);

    //
    //
    //

    hr = shader.create_D3D11_buffer(&buf1, (const void*)screen_mem, sizeof(unsigned int) * width * height, sizeof(unsigned int), D3D11_BIND_UNORDERED_ACCESS, 0, 0, D3D11_USAGE_DEFAULT);
    if (FAILED(hr)) { return 3; }

    hr = shader.create_D3D11_buffer(&buf2, (const void*)screen_z, sizeof(float) * width * height, sizeof(float), D3D11_BIND_UNORDERED_ACCESS, 0, 0, D3D11_USAGE_DEFAULT);
    if (FAILED(hr)) { return 4; }

    hr = shader.create_D3D11_buffer(&buf3, (const void*)frag_buf, sizeof(Fragment) * width * height, sizeof(Fragment), D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_SHADER_RESOURCE, D3D11_RESOURCE_MISC_BUFFER_STRUCTURED, 0, D3D11_USAGE_DEFAULT);
    if (FAILED(hr)) { return 4; }

    ID3D11Buffer* buf4;
    hr = shader.create_D3D11_buffer(&buf4, (const void*)positions, sizeof(float) * 9 * triangle_count, sizeof(float), D3D11_BIND_SHADER_RESOURCE, D3D11_RESOURCE_MISC_BUFFER_STRUCTURED, 0, D3D11_USAGE_DEFAULT);
    if (FAILED(hr)) { return 5; }

    ID3D11Buffer* buf5;
    hr = shader.create_D3D11_buffer(&buf5, (const void*)colors, sizeof(float) * 13 * triangle_count, sizeof(float), D3D11_BIND_SHADER_RESOURCE, D3D11_RESOURCE_MISC_BUFFER_STRUCTURED, 0, D3D11_USAGE_DEFAULT);
    if (FAILED(hr)) { return 6; }

    ID3D11Buffer* buf6;
    hr = shader.create_D3D11_buffer(&buf6, (const void*)misc, sizeof(unsigned int) * 3, sizeof(unsigned int), D3D11_BIND_SHADER_RESOURCE, D3D11_RESOURCE_MISC_BUFFER_STRUCTURED, 0, D3D11_USAGE_DEFAULT);
    if (FAILED(hr)) { return 7; }

    ID3D11Buffer* buf7;
    hr = shader.create_D3D11_buffer(&buf7, (const void*)tri_meta.data(), sizeof(float) * 5 * triangle_count, sizeof(float), D3D11_BIND_SHADER_RESOURCE, D3D11_RESOURCE_MISC_BUFFER_STRUCTURED, 0, D3D11_USAGE_DEFAULT);
    if (FAILED(hr)) { return 7; }

    shader.compile_shader(L"rasterizer.hlsl", "main");
    hr = device->CreateComputeShader(shader.get_shader_blob()->GetBufferPointer(), shader.get_shader_blob()->GetBufferSize(), nullptr, &compute_shader);

    if (FAILED(hr)) { return hr; }

    ID3D11UnorderedAccessView* UAV1 = nullptr;
    hr = shader.create_UAV(buf1, &UAV1, width * height, DXGI_FORMAT_R32_UINT);
    if (FAILED(hr)) { std::cout << "Error creating D3D11 UAV\n" << std::hex << hr << "\n"; return -4; }

    ID3D11UnorderedAccessView* UAV2 = nullptr;
    hr = shader.create_UAV(buf2, &UAV2, width * height, DXGI_FORMAT_R32_FLOAT);
    if (FAILED(hr)) { std::cout << "Error creating D3D11 UAV\n" << std::hex << hr << "\n"; return -4; }

    ID3D11UnorderedAccessView* UAV3 = nullptr;
    hr = shader.create_UAV(buf3, &UAV3, width * height, DXGI_FORMAT_UNKNOWN);
    if (FAILED(hr)) { std::cout << "Error creating D3D11 UAV\n" << std::hex << hr << "\n"; return -80; }

    ID3D11UnorderedAccessView* UAVs[] = { UAV1, UAV2, UAV3 };

    ID3D11ShaderResourceView* SRV1 = nullptr;
    hr = shader.create_SRV(buf4, &SRV1, 9 * triangle_count, D3D11_SRV_DIMENSION_BUFFER);
    if (FAILED(hr)) { std::cout << "Error creating D3D11 SRV\n" << std::hex << hr << "\n"; return -5; }

    ID3D11ShaderResourceView* SRV2 = nullptr;
    hr = shader.create_SRV(buf5, &SRV2, 13 * triangle_count, D3D11_SRV_DIMENSION_BUFFER);
    if (FAILED(hr)) { std::cout << "Error creating D3D11 SRV\n" << std::hex << hr << "\n"; return -5; }

    ID3D11ShaderResourceView* SRV3 = nullptr;
    hr = shader.create_SRV(buf6, &SRV3, 3, D3D11_SRV_DIMENSION_BUFFER);
    if (FAILED(hr)) { std::cout << "Error creating D3D11 SRV\n" << std::hex << hr << "\n"; return -5; }
    
    ID3D11ShaderResourceView* SRV4 = nullptr;
    hr = shader.create_SRV(buf7, &SRV4, 5 * triangle_count, D3D11_SRV_DIMENSION_BUFFER);
    if (FAILED(hr)) { std::cout << "Error creating D3D11 SRV\n" << std::hex << hr << "\n"; return -5; }

    ID3D11ShaderResourceView* SRVs[] = { SRV1, SRV2, SRV3, SRV4 };
    
    hr = shader.create_D3D11_buffer(&CPU_buf, nullptr, sizeof(unsigned int) * width * height, sizeof(unsigned int), 0, 0, D3D11_CPU_ACCESS_READ, D3D11_USAGE_STAGING);
    if (FAILED(hr)) { std::cout << "Error creating D3D11 CPU buffer\n"; return -5; }


    ctx->CSSetShader(compute_shader, nullptr, 0); // Bind the compute shader
    ctx->CSSetUnorderedAccessViews(0, 3, UAVs, nullptr); // Bind UAV to slot u0
    ctx->CSSetShaderResources(0, 4, SRVs);

    //
    //
    //

    MSG msg = {};
    while (running) {
        // Check Windows messages
        if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        auto start = std::chrono::high_resolution_clock::now();
        
        for (unsigned int i = 0; i < triangle_count; ++i) {
            unsigned int minx = std::min({ positions[i * 9], positions[i * 9 + 3], positions[i * 9 + 6] });
            unsigned int miny = std::min({ positions[i * 9 + 1], positions[i * 9 + 4], positions[i * 9 + 7] });
            unsigned int maxx = std::max({ positions[i * 9], positions[i * 9 + 3], positions[i * 9 + 6] });
            unsigned int maxy = std::max({ positions[i * 9 + 1], positions[i * 9 + 4], positions[i * 9 + 7] });

            float inv_area = 1.0f / ((positions[i * 9] - positions[i * 9 + 3]) * (positions[i * 9 + 7] - positions[i * 9 + 4]) - (positions[i * 9 + 1] - positions[i * 9 + 4]) * (positions[i * 9 + 6] - positions[i * 9 + 3]));

            tri_meta.push_back((float)minx);
            tri_meta.push_back((float)miny);
            tri_meta.push_back((float)maxx);
            tri_meta.push_back((float)maxy);
            tri_meta.push_back(inv_area);
        }
        
        ctx->UpdateSubresource(buf7, 0, nullptr, tri_meta.data(), 0, 0);
        ctx->Dispatch(ceil((144 * triangle_count) / 256.0f), 1, 1);
        ctx->Flush();

        retreive = shader.retreive_D3D11_process<unsigned int>(CPU_buf, buf1);
        if (retreive) memcpy(screen_mem, retreive, sizeof(unsigned int) * width * height);

        auto end = std::chrono::high_resolution_clock::now();

        std::chrono::duration<double, std::milli> elapsed = end - start;
        std::cout << "Perf: " << 1000 / elapsed.count() << " fps\n";
    }

    if (compute_shader) compute_shader->Release();
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

    return 0;
}
