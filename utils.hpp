#pragma once
#include <d3d11.h>
#include <d3dcompiler.h>
#include <Windows.h>
#include <iostream>
#include <vector>
#include <fstream>
#include <chrono>
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "d3dcompiler.lib")
#define PI 3.14159

float radians(float degrees) {
	return PI * (degrees / 180);
}

template<typename T, unsigned int A, unsigned int B, unsigned int D> std::array<T, A* D> matrix_mul(const std::array<T, A* B>& x, const std::array<T, B* D>& y) {
	std::array<T, A* D> result = {};

	for (unsigned int i = 0; i < A; ++i) {
		for (unsigned int j = 0; j < D; ++j) {
			T sum = 0;
			for (unsigned int k = 0; k < B; ++k) {
				sum += x[i * B + k] * y[k * D + j];
			}
			result[i * D + j] = sum;
		}
	}

	return result;
}

template<typename T> std::vector<T> add_em(T* x, int size, int more) { // Add empty memory!
	std::vector<T> y(size + more);

	for (unsigned int i = 0; i < (size / sizeof(T)); i++) {
		y[i] = x[i];
	}

	for (unsigned int i = size / sizeof(T); i < (more / sizeof(T) + size / sizeof(T)); i++) {
		y[i] = 1;
	}

	return y;
}

void attach_console() {
	if (AllocConsole()) {
		FILE* file;
		freopen_s(&file, "CONOUT$", "w", stdout);
	}
}

#include <iostream>
#include <fstream>
#include <vector>
#include <cstdint>

std::vector<unsigned int> read_bmp(std::string file_name) {
	std::ifstream file(file_name, std::ios::in | std::ios::binary);
	std::vector<unsigned int> img;

	if (!file.is_open()) {
		std::cout << "Error: File could not be opened!\n";
		return { 0 };
	}

	char header[54];
	file.read(header, 54);

	if (header[0] != 'B' || header[1] != 'M') {
		std::cout << "Error: Not a valid BMP file!\n";
		return { 0 };
	}

	int width = *(int*)&header[18];
	int height = *(int*)&header[22];
	short bpp = *(short*)&header[28];

	if (bpp != 24 && bpp != 32) {
		std::cout << "Error: Only 24-bit or 32-bit BMP files are supported!\n";
		return { 0 };
	}

	int bytes_per_pixel = bpp / 8;
	int row_padded = (width * bytes_per_pixel + 3) & (~3);

	std::vector<unsigned char> data(row_padded * height);
	file.read(reinterpret_cast<char*>(data.data()), data.size());

	img.resize(2 + width * height);
	img[0] = width;
	img[1] = height;

	int pixel_idx = 2;

	for (int y = 0; y < height; y++) {
		for (int x = 0; x < width; x++) {
			int i = y * row_padded + x * bytes_per_pixel;

			unsigned char b = data[i + 0];
			unsigned char g = data[i + 1];
			unsigned char r = data[i + 2];
			unsigned char a = (bpp == 32) ? data[i + 3] : 255; // default alpha if 24-bit

			// pack RGBA -> 0xRRGGBBAA
			unsigned int pixel = (r << 24) | (g << 16) | (b << 8) | a;
			img[pixel_idx++] = pixel;
		}
	}

	return img;
}

class Shader {
private:
	HRESULT hr;
	ID3D11Device* device;
	D3D_FEATURE_LEVEL level;
	ID3D11DeviceContext* ctx;
	ID3DBlob* error_blob;
	ID3DBlob* shader_blob;
public:
	Shader(void) {};
	~Shader() {
		if (shader_blob) shader_blob->Release();
	}

	void Shader_device(ID3D11Device* dev, ID3D11DeviceContext* ctx_, D3D_FEATURE_LEVEL features) {
		device = dev;
		ctx = ctx_;
		level = features;
	}

	HRESULT create_D3D11_buffer(ID3D11Buffer** buf, const void* dat_in, unsigned int size, unsigned char item_size, unsigned int type, unsigned int flags, unsigned int CPU_flags, D3D11_USAGE usage) {
		D3D11_BUFFER_DESC buffer_description = { };
		buffer_description.ByteWidth = size;
		buffer_description.StructureByteStride = item_size;
		buffer_description.BindFlags = type;
		buffer_description.CPUAccessFlags = CPU_flags;
		buffer_description.MiscFlags = flags;
		buffer_description.Usage = usage;

		D3D11_SUBRESOURCE_DATA data_in = { };
		if (dat_in) {
			data_in.pSysMem = dat_in;
		}

		return device->CreateBuffer(&buffer_description, dat_in ? &data_in : nullptr, buf);
	}

	HRESULT compile_shader(LPCWSTR file_name, LPCSTR entrypoint) {
		ID3DBlob* error_blob;
		return D3DCompileFromFile(file_name, nullptr, nullptr, entrypoint, "cs_5_0", 0, 0, &shader_blob, &error_blob);
	}

	ID3DBlob* get_error_blob() {
		return error_blob;
	}

	ID3DBlob* get_shader_blob() {
		return shader_blob;
	}

	HRESULT create_UAV(ID3D11Resource* buf, ID3D11UnorderedAccessView** UAV, unsigned int size, DXGI_FORMAT format) {
		D3D11_UNORDERED_ACCESS_VIEW_DESC UAV_desc = { };
		UAV_desc.Format = format;
		UAV_desc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
		UAV_desc.Buffer.FirstElement = 0;
		UAV_desc.Buffer.NumElements = size;

		return device->CreateUnorderedAccessView(buf, &UAV_desc, UAV);
	}

	HRESULT create_SRV(ID3D11Resource* buf, ID3D11ShaderResourceView** SRV, unsigned int size, D3D11_SRV_DIMENSION dimension) {
		D3D11_SHADER_RESOURCE_VIEW_DESC SRV_desc = { };
		SRV_desc.Format = DXGI_FORMAT_UNKNOWN;
		SRV_desc.ViewDimension = dimension;
		SRV_desc.Buffer.NumElements = size;
		SRV_desc.Buffer.FirstElement = 0;

		return device->CreateShaderResourceView(buf, &SRV_desc, SRV);
	}

	template<typename T> T* retrieve_D3D11_process(ID3D11Resource* CPU_buf, ID3D11Resource* GPU_buf) {
		D3D11_MAPPED_SUBRESOURCE map_r = { };
		ctx->CopyResource(CPU_buf, GPU_buf);

		HRESULT hr = ctx->Map(CPU_buf, 0, D3D11_MAP_READ, 0, &map_r);

		if (FAILED(hr)) {
			std::cout << "Mapping failed. HRESULT: " << std::hex << hr << "\n";
			return 0;
		}

		T* dat = (T*)map_r.pData;
		ctx->Unmap(CPU_buf, 0);

		return dat;
	}
};
