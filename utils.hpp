#pragma once
#include <d3d11.h>
#include <d3dcompiler.h>
#include <Windows.h>
#include <iostream>
#include <vector>
#include <fstream>
#include <chrono>
#include <cstdint>
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")
#define PI 3.14159

struct Triangle {
	float data[23];
};

struct mip_resources {
	std::vector<ID3D11Texture2D*> mip_buffers;
	std::vector<ID3D11Texture2D*> readback_buffers;
	std::vector<ID3D11UnorderedAccessView*> mip_UAVs;

	ID3D11Buffer* level_buf;
	ID3D11ShaderResourceView* level_SRV;
};

float mean(std::vector<float> items) {
	float res = 0;
	for (unsigned int i = 0; i < items.size(); i++) {
		res += items[i];
	}

	return res / items.size();
}

float radians(float degrees) {
	return PI * (degrees / 180);
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

void final_buf(unsigned int* output, unsigned int* frame_buf1, unsigned int* frame_buf2, unsigned int* depth_buf1, unsigned int* depth_buf2, unsigned int size) {
	for (unsigned int i = 0; i < size; i++) {
		if (i < size / 2) {
			output[i] = frame_buf1[i];
		} else {
			output[i] = frame_buf2[i];
		}
	}
}

std::string read_file(const char* filename) {
	std::ifstream file(filename);

	if (!file.is_open()) {
		std::cerr << "Error opening file: " << filename << std::endl;
		return 0;
	}

	std::string contents = "";
	std::string line = "";

	while (std::getline(file, line)) {
		contents += line;
		contents += "\n";
	}

	return contents;
}

class Shader {
private:
	HRESULT hr;
	ID3D11Device* device;
	ID3D11DeviceContext* ctx;
	ID3DBlob* error_blob;
	ID3DBlob* shader_blob;
public:
	Shader(void) {};

	void Shader_device(ID3D11Device* dev, ID3D11DeviceContext* ctx_) {
		device = dev;
		ctx = ctx_;
	}

	HRESULT create_D3D11_buffer(ID3D11Buffer** buf, const void* dat_in, unsigned int size, unsigned char item_size, unsigned int bind, unsigned int flags, unsigned int CPU_flags, D3D11_USAGE usage) {
		D3D11_BUFFER_DESC buffer_description = { };
		buffer_description.ByteWidth = size;
		buffer_description.StructureByteStride = item_size;
		buffer_description.BindFlags = bind;
		buffer_description.CPUAccessFlags = CPU_flags;
		buffer_description.MiscFlags = flags;
		buffer_description.Usage = usage;

		D3D11_SUBRESOURCE_DATA data_in = { };
		if (dat_in) {
			data_in.pSysMem = dat_in;
		}

		return device->CreateBuffer(&buffer_description, dat_in ? &data_in : nullptr, buf);
	}

	HRESULT compile_shader(LPCWSTR file_name, LPCSTR entrypoint, LPCSTR model) {
		ID3DBlob* error_blob;
		return D3DCompileFromFile(file_name, nullptr, nullptr, entrypoint, model, D3DCOMPILE_OPTIMIZATION_LEVEL3, 0, &shader_blob, &error_blob);
	}

	ID3DBlob* get_error_blob() {
		return error_blob;
	}

	ID3DBlob* get_shader_blob() {
		return shader_blob;
	}

	HRESULT create_UAV(ID3D11Resource* buf, ID3D11UnorderedAccessView** UAV, unsigned int size, DXGI_FORMAT format, unsigned int flags, D3D11_UAV_DIMENSION dimension) {
		D3D11_UNORDERED_ACCESS_VIEW_DESC UAV_desc = { };
		UAV_desc.Format = format;
		UAV_desc.ViewDimension = dimension;
		UAV_desc.Buffer.FirstElement = 0;
		UAV_desc.Buffer.NumElements = size;
		UAV_desc.Buffer.Flags = flags;

		return device->CreateUnorderedAccessView(buf, &UAV_desc, UAV);
	}

	HRESULT create_SRV(ID3D11Resource* buf, ID3D11ShaderResourceView** SRV, unsigned int size, DXGI_FORMAT format, D3D11_SRV_DIMENSION dimension) {
		D3D11_SHADER_RESOURCE_VIEW_DESC SRV_desc = { };
		SRV_desc.Format = format;
		SRV_desc.ViewDimension = dimension;
		SRV_desc.Buffer.NumElements = size;
		SRV_desc.Buffer.FirstElement = 0;

		return device->CreateShaderResourceView(buf, &SRV_desc, SRV);
	}

	template<typename T> void retrieve_D3D11_process(T* dest, ID3D11Resource* CPU_buf, ID3D11Resource* GPU_buf, ID3D11DeviceContext* ctx_temp, bool region, unsigned int mip_dst, unsigned int mip_src, unsigned int size) {
		D3D11_MAPPED_SUBRESOURCE map_r = { };
		
		if (region) {
			ctx->CopySubresourceRegion(
				CPU_buf,
				mip_dst, // dst mip
				0, 0, 0,
				GPU_buf,
				mip_src, // src mip
				nullptr
			);
		} else {
			ctx_temp->CopyResource(CPU_buf, GPU_buf);
		}

   		HRESULT hr = ctx_temp->Map(CPU_buf, 0, D3D11_MAP_READ, 0, &map_r);

		if (FAILED(hr)) {
			std::cout << "Mapping failed. HRESULT: " << std::hex << hr << "\n";
		}

		std::memcpy(dest, map_r.pData, size);
		ctx_temp->Unmap(CPU_buf, 0);
	}
};

void CPU_vertex_transformation(std::vector<float>& sogp, std::vector<float>& spositions, unsigned int triangle_count_1, float thetax, float thetay) {
	for (unsigned int i = 0; i < triangle_count_1 * 9; i += 3) {
		float x = sogp[i];
		float y = sogp[i + 1];
		float z = 0.1f * (sogp[i + 2] + 10);

		float ry = radians(thetax);
		float rx = radians(thetay);

		float y1 = y * cos(rx) - z * sin(rx);
		float z1 = y * sin(rx) + z * cos(rx);
		float x1 = x;

		float xr = x1 * cos(ry) + z1 * sin(ry);
		float yr = y1;
		float zr = -x1 * sin(ry) + z1 * cos(ry);

		float invz = 1.0f / zr;
		float xp = (xr * invz * 0.5f + 0.5f);
		float yp = (yr * invz * -0.5f + 0.5f);

		spositions[i] = xp;
		spositions[i + 1] = yp;
		spositions[i + 2] = zr;
	}

	for (unsigned int i = 0; i < triangle_count_1; i++) {
		if (spositions[i * 9 + 2] <= 0.25 || spositions[i * 9 + 5] <= 0.25 || spositions[i * 9 + 8] <= 0.25) {
			spositions[i * 9] = 0;
			spositions[i * 9 + 1] = 0;
			spositions[i * 9 + 2] = 0;
			spositions[i * 9 + 3] = 0;
			spositions[i * 9 + 4] = 0;
			spositions[i * 9 + 5] = 0;
			spositions[i * 9 + 6] = 0;
			spositions[i * 9 + 7] = 0;
			spositions[i * 9 + 8] = 0;
		}
	}
}

void CPU_sort_shader(std::vector<float>& spositions, std::vector<float>& scolors, std::vector<Triangle>& triangles, unsigned int triangle_count_1) {
	for (unsigned int i = 0; i < triangle_count_1; i++) {
		triangles[i] = {
			{
				spositions[i * 9],
				spositions[i * 9 + 1],
				spositions[i * 9 + 2],
				spositions[i * 9 + 3],
				spositions[i * 9 + 4],
				spositions[i * 9 + 5],
				spositions[i * 9 + 6],
				spositions[i * 9 + 7],
				spositions[i * 9 + 8],
				scolors[i * 13],
				scolors[i * 13 + 1],
				scolors[i * 13 + 2],
				scolors[i * 13 + 3],
				scolors[i * 13 + 4],
				scolors[i * 13 + 5],
				scolors[i * 13 + 6],
				scolors[i * 13 + 7],
				scolors[i * 13 + 8],
				scolors[i * 13 + 9],
				scolors[i * 13 + 10],
				scolors[i * 13 + 11],
				scolors[i * 13 + 12],
				((spositions[i * 9 + 8] * 10 - 10) + (spositions[i * 9 + 5] * 10 - 10) + (spositions[i * 9 + 2] * 10 - 10)) / 3.0f
			}
		};
	}

	std::sort(triangles.begin(), triangles.end(),
		[](const Triangle& a, const Triangle& b) {
			return a.data[22] < b.data[22];
		});

	for (unsigned int i = 0; i < triangle_count_1; i++) {
		spositions[i * 9] = triangles[i].data[0];
		spositions[i * 9 + 1] = triangles[i].data[1];
		spositions[i * 9 + 2] = triangles[i].data[2];
		spositions[i * 9 + 3] = triangles[i].data[3];
		spositions[i * 9 + 4] = triangles[i].data[4];
		spositions[i * 9 + 5] = triangles[i].data[5];
		spositions[i * 9 + 6] = triangles[i].data[6];
		spositions[i * 9 + 7] = triangles[i].data[7];
		spositions[i * 9 + 8] = triangles[i].data[8];
		scolors[i * 13] = triangles[i].data[9];
		scolors[i * 13 + 1] = triangles[i].data[10];
		scolors[i * 13 + 2] = triangles[i].data[11];
		scolors[i * 13 + 3] = triangles[i].data[12];
		scolors[i * 13 + 4] = triangles[i].data[13];
		scolors[i * 13 + 5] = triangles[i].data[14];
		scolors[i * 13 + 6] = triangles[i].data[15];
		scolors[i * 13 + 7] = triangles[i].data[16];
		scolors[i * 13 + 8] = triangles[i].data[17];
		scolors[i * 13 + 9] = triangles[i].data[18];
		scolors[i * 13 + 10] = triangles[i].data[19];
		scolors[i * 13 + 11] = triangles[i].data[20];
		scolors[i * 13 + 12] = triangles[i].data[21];
	}
}

void CPU_triangle_metadata(std::vector<float>& tri_meta, std::vector<float>& spositions, unsigned int triangle_count_1, unsigned short width, unsigned short height) {
	for (unsigned int i = 0; i < triangle_count_1; i++) {
		spositions[i * 9] *= width;
		spositions[i * 9 + 1] *= height;
		spositions[i * 9 + 3] *= width;
		spositions[i * 9 + 4] *= height;
		spositions[i * 9 + 6] *= width;
		spositions[i * 9 + 7] *= height;

		int minx = std::min(spositions[i * 9], std::min(spositions[i * 9 + 3], spositions[i * 9 + 6]));
		int miny = std::min(spositions[i * 9 + 1], std::min(spositions[i * 9 + 4], spositions[i * 9 + 7]));
		int maxx = std::max(spositions[i * 9], std::max(spositions[i * 9 + 3], spositions[i * 9 + 6]));
		int maxy = std::max(spositions[i * 9 + 1], std::max(spositions[i * 9 + 4], spositions[i * 9 + 7]));

		float inv_area = 1.0f / ((spositions[i * 9] - spositions[i * 9 + 3]) * (spositions[i * 9 + 7] - spositions[i * 9 + 4]) - (spositions[i * 9 + 1] - spositions[i * 9 + 4]) * (spositions[i * 9 + 6] - spositions[i * 9 + 3]));
		float v3ymv2y = spositions[i * 9 + 7] - spositions[i * 9 + 4];
		float v1ymv3y = spositions[i * 9 + 1] - spositions[i * 9 + 7];
		float v3xmv2x = spositions[i * 9 + 6] - spositions[i * 9 + 3];
		float v1xmv3x = spositions[i * 9] - spositions[i * 9 + 6];

		tri_meta.push_back((float)minx);
		tri_meta.push_back((float)miny);
		tri_meta.push_back((float)maxx);
		tri_meta.push_back((float)maxy);
		tri_meta.push_back(inv_area);
		tri_meta.push_back(v3ymv2y);
		tri_meta.push_back(v1ymv3y);
		tri_meta.push_back(v3xmv2x);
		tri_meta.push_back(v1xmv3x);
		tri_meta.push_back(v3ymv2y * inv_area);
		tri_meta.push_back(-v3xmv2x * inv_area);
		tri_meta.push_back(v1ymv3y * inv_area);
		tri_meta.push_back(-v1xmv3x * inv_area);
	}
}

struct triangle_everything {
	std::vector<float> positions;
	std::vector<float> colors;
	std::vector<float> tri_meta;
	unsigned int triangles_passed;
};

triangle_everything passed_filter(std::vector<float> spositions, std::vector<float> scolors, std::vector<float> tri_meta, std::vector<unsigned int> occluder_results) {
	triangle_everything result = {};
	std::vector<float> positions;
	std::vector<float> colors;
	std::vector<float> meta;
	unsigned int passed = 0;

	for (unsigned int i = 0; i < occluder_results.size(); i++) {
		if (occluder_results[i] == 1) {
			for (unsigned int j = 0; j < 9; j++) {
				positions.push_back(spositions[i * 9 + j]);
			}
			for (unsigned int j = 0; j < 13; j++) {
				colors.push_back(scolors[i * 13 + j]);
				meta.push_back(tri_meta[i * 13 + j]);
			}
			passed++;
		}
	}

	result.colors = colors;
	result.positions = positions;
	result.triangles_passed = passed;
	result.tri_meta = meta;

	return result;
}
