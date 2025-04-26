#define NOMINMAX
#include <iostream>
#include <Windows.h>
#include <chrono>
#include <algorithm>
#include <fstream>
#include <vector>
#include "utils.hpp"
#pragma comment(linker, "/SUBSYSTEM:WINDOWS")

bool running = true;
unsigned short width = 0;
unsigned short height = 0;
void* screen_mem = malloc(1920 * 1080 * sizeof(unsigned int));
void* screen_z = malloc(1920 * 1080 * sizeof(float));

std::vector<unsigned int> read_bmp(std::string file_name) {
    std::ifstream file(file_name, std::ios::in | std::ios::binary);

    if (!file.is_open()) {
        std::cout << "Error: File could not be opened!" << std::endl;
        std::vector<unsigned int> error(1);
        return error;
    }

    unsigned char header[54];
    file.read(reinterpret_cast<char*>(header), 54);

    unsigned int width = *reinterpret_cast<unsigned int*>(&header[18]);
    unsigned int height = *reinterpret_cast<unsigned int*>(&header[22]);
    unsigned int depth = *reinterpret_cast<unsigned short*>(&header[28]);
    unsigned int size = ((depth * width + 31) / 32) * 4;
    std::vector<unsigned char> pixel_data(size * height);
    
    file.read(reinterpret_cast<char*>(pixel_data.data()), size * height);
    file.close();

    std::vector<unsigned int> tex(width * height + 2);
    tex[0] = width;
    tex[1] = height;

    for (unsigned int y = 0; y < height; y++) {
        for (unsigned int x = 0; x < width; x++) {
            // BMPs are stored in BGR format
            unsigned int idx = y * depth + x * (depth / 8);
            unsigned char b = pixel_data[idx];
            unsigned char g = pixel_data[idx + 1];
            unsigned char r = pixel_data[idx + 2];

            // Convert to RGBA format for the texture
            tex[2 + (height - 1 - y) * width + x] = (r << 16) | (g << 8) | b;
        }
    }

    return tex;
}

std::vector<unsigned int> create_test_texture(int width, int height) {
    std::vector<unsigned int> tex(width * height + 2);
    tex[0] = width;
    tex[1] = height;

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            bool isWhite = ((x / 8) % 2) == ((y / 8) % 2);
            tex[2 + y * width + x] = isWhite ? 0xFFFFFF : 0x0000FF; // White or blue
        }
    }
    return tex;
}

template <typename T> T clamp(T value, T min, T max) {
    if (value < min) return min;
    if (value > max) return max;
    return value;
}

struct RGB {
    unsigned int r, g, b;
};

LRESULT w_proc(HWND window, UINT msg, WPARAM wparam, LPARAM lparam) {
    switch (msg) {
        case WM_SIZE: {
            width = LOWORD(lparam);
            height = HIWORD(lparam);

            screen_mem = realloc(screen_mem, sizeof(unsigned int) * width * height);
            if (screen_mem == nullptr) {
                std::cerr << "Failed to allocate screen memory.";
                exit(EXIT_FAILURE); // Handle error gracefully
            }

            screen_z = realloc(screen_z, sizeof(float) * width * height);
            if (screen_z == nullptr) {
                std::cerr << "Failed to allocate screen memory.";
                exit(EXIT_FAILURE); // Handle error gracefully
            }

            break;
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

                StretchDIBits(hdc, 0, 0, width, height, 0, 0, width, height, screen_mem, &bmi, DIB_RGB_COLORS, SRCCOPY);
            }

            EndPaint(window, &ps);
            break;
        }

        case WM_DESTROY: {
            if (screen_mem) free(screen_mem);
            if (screen_z) free(screen_z);
            running = false;
            PostQuitMessage(0);
            return 0;
        }

        default: {
            return DefWindowProc(window, msg, wparam, lparam);
        }
    }
}

int WinMain(HINSTANCE h_inst, HINSTANCE p_inst, LPSTR lpcmdln, int n_cmd_show) {
    attach_console();

    unsigned int triangle_count = 3;
    ID3D11Device* device = nullptr;
    ID3D11DeviceContext* ctx = nullptr;
    D3D_FEATURE_LEVEL level;
    ID3D11ComputeShader* compute_shader;
    WNDCLASS w_class = { 0 };

    w_class.lpszClassName = L"class";
    w_class.lpfnWndProc = w_proc;
    w_class.hInstance = h_inst;

    RegisterClassW(&w_class);

    HWND wnd = CreateWindowExW(0, L"class", L"My game engine, with just the windows API", WS_OVERLAPPEDWINDOW, 0, 0, 1920, 1080, 0, 0, h_inst, 0);
    ShowWindow(wnd, n_cmd_show);

    for (int i = 0; i < width * height; i++) {
        ((float*)screen_z)[i] = FLT_MAX; // Start with maximum depth (farthest)
    }

    // THE POSITIONS NEED TO BE CLOCKWISE (CW)
    unsigned int positions[] = {
        300, 400, 10,
        1000, 100, 10,
        1600, 1000, 10,
        
        100, 200, 20,
        1000, 200, 20,
        300, 600, 20,
        
        0, 0, 30,
        600, 600, 30,
        0, 600, 30
    };

    float colors[] = {
        1.0f, 0.0f, 0.0f, 0.1f,
        0.0f, 1.0f, 0.0f, 0.8f,
        0.0f, 0.0f, 1.0f, 0.1f, 1.0f,
        
        0.0f, 1.0f, 1.0f, 1.0f,
        0.5f, 1.0f, 0.0f, 1.0f,
        0.5f, 0.0f, 1.0f, 1.0f, 1.0f,

        0.0f, 0.0f, 0.0f, 1.0f,
        0.5f, 1.0f, 0.0f, 1.0f,
        0.0f, 1.0f, 0.0f, 1.0f, 0.0f
    };

    unsigned int misc[] = {
        width, height, triangle_count
    }; // Screen width, screen height, triangle count

    std::vector<unsigned int> tex_vector = create_test_texture(256, 256);
    unsigned int* tex = tex_vector.data();

    std::cout << "Texture size: " << tex[0] << "x" << tex[1] << std::endl;
    std::cout << "Top-left pixel: " << std::hex << tex[2] << std::dec << std::endl;
    std::cout << "Center pixel: " << std::hex << tex[2 + (tex[1] / 2) * tex[0] + tex[0] / 2] << std::dec << std::endl;
    std::cout << "Bottom-right pixel: " << std::hex << tex[2 + (tex[1] - 1) * tex[0] + tex[0] - 1] << std::dec << std::endl;

    Shader shader;
    HRESULT hr = D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0, nullptr, 0, D3D11_SDK_VERSION, &device, &level, &ctx);
    if (FAILED(hr)) { return -1; }
    shader.Shader_device(device, ctx, level);

    ID3D11Buffer* buf1;
    hr = shader.create_D3D11_buffer(&buf1, (const void*)screen_mem, sizeof(unsigned int) * width * height, sizeof(unsigned int), D3D11_BIND_UNORDERED_ACCESS, 0, 0, D3D11_USAGE_DEFAULT);
    if (FAILED(hr)) { return hr; }

    ID3D11Buffer* buf2;
    hr = shader.create_D3D11_buffer(&buf2, (const void*)screen_z, sizeof(float) * width * height, sizeof(float), D3D11_BIND_UNORDERED_ACCESS, 0, 0, D3D11_USAGE_DEFAULT);
    if (FAILED(hr)) { return hr; }

    ID3D11Buffer* buf3;
    hr = shader.create_D3D11_buffer(&buf3, (const void*)positions, sizeof(positions), sizeof(unsigned int), D3D11_BIND_UNORDERED_ACCESS, 0, 0, D3D11_USAGE_DEFAULT);
    if (FAILED(hr)) { return hr; }

    ID3D11Buffer* buf4;
    hr = shader.create_D3D11_buffer(&buf4, (const void*)colors, sizeof(colors), sizeof(float), D3D11_BIND_UNORDERED_ACCESS, 0, 0, D3D11_USAGE_DEFAULT);
    if (FAILED(hr)) { return hr; }

    ID3D11Buffer* buf5;
    hr = shader.create_D3D11_buffer(&buf5, (const void*)misc, sizeof(misc), sizeof(unsigned int), D3D11_BIND_UNORDERED_ACCESS, 0, 0, D3D11_USAGE_DEFAULT);
    if (FAILED(hr)) { return hr; }

    ID3D11Buffer* buf6;
    hr = shader.create_D3D11_buffer(&buf6, (const void*)tex, sizeof(unsigned int) * (tex[0] * tex[1] + 2), sizeof(unsigned int), D3D11_BIND_UNORDERED_ACCESS, 0, 0, D3D11_USAGE_DEFAULT);
    if (FAILED(hr)) { return hr; }

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
    hr = shader.create_UAV(buf3, &UAV3, sizeof(positions) / sizeof(unsigned int), DXGI_FORMAT_R32_UINT);
    if (FAILED(hr)) { std::cout << "Error creating D3D11 UAV\n" << std::hex << hr << "\n"; return -4; }

    ID3D11UnorderedAccessView* UAV4 = nullptr;
    hr = shader.create_UAV(buf4, &UAV4, sizeof(colors) / sizeof(float), DXGI_FORMAT_R32_FLOAT);
    if (FAILED(hr)) { std::cout << "Error creating D3D11 UAV\n" << std::hex << hr << "\n"; return -4; }

    ID3D11UnorderedAccessView* UAV5 = nullptr;
    hr = shader.create_UAV(buf5, &UAV5, 3, DXGI_FORMAT_R32_UINT);
    if (FAILED(hr)) { std::cout << "Error creating D3D11 UAV\n" << std::hex << hr << "\n"; return -4; }

    ID3D11UnorderedAccessView* UAV6 = nullptr;
    hr = shader.create_UAV(buf6, &UAV6, tex[0] * tex[1] + 2, DXGI_FORMAT_R32_UINT);
    if (FAILED(hr)) { std::cout << "Error creating D3D11 UAV\n" << std::hex << hr << "\n"; return hr; }

    ID3D11UnorderedAccessView* UAVs[] = { UAV1, UAV2, UAV3, UAV4, UAV5, UAV6 };
    ctx->CSSetShader(compute_shader, nullptr, 0); // Bind the compute shader
    ctx->CSSetUnorderedAccessViews(0, 6, UAVs, nullptr); // Bind UAV to slot u0

    ID3D11Buffer* CPU_buf;
    hr = shader.create_D3D11_buffer(&CPU_buf, nullptr, sizeof(unsigned int) * width * height, sizeof(unsigned int), 0, 0, D3D11_CPU_ACCESS_READ, D3D11_USAGE_STAGING);
    if (FAILED(hr)) { std::cout << "Error creating D3D11 CPU buffer\n"; return -5; }

    auto start_ = std::chrono::high_resolution_clock::now();

    ctx->Dispatch(ceil((double)(triangle_count / 1024.0f)), 1, 1);
    ctx->Flush();

    auto end_ = std::chrono::high_resolution_clock::now();

    ID3D11UnorderedAccessView* null_UAV = nullptr;
    ctx->CSSetUnorderedAccessViews(0, 1, &null_UAV, nullptr);
    ctx->CSSetShader(nullptr, nullptr, 0);

    unsigned int* retreive = shader.retreive_D3D11_process<unsigned int>(CPU_buf, buf1);
    if (retreive == 0) { std::cout << "Error mapping D3D11 data\n"; return -6; }
    if (retreive == nullptr) {
        return -7;
    }

    memcpy(screen_mem, retreive, width * height * sizeof(unsigned int));

    std::chrono::duration<double, std::milli> elapsed = end_ - start_;
    std::cout << "D3D11 IDIOT took: " << elapsed.count() << " ms\n";

    MSG msg = {};
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    if (compute_shader) compute_shader->Release();
    if (ctx) ctx->Release();
    if (device) device->Release();

	return 0;
}
