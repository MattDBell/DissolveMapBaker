#include "ComputeDevice.h"

// direct3d 
#include <d3d11.h>
#include <d3dcompiler.h>
#include <dxgi.h>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "dxgi.lib")

using std::string;

// Global State
ID3D11Device* device;
ID3D11DeviceContext* deviceContext;
D3D_FEATURE_LEVEL featureLevel;

ComputeDevice::ComputeDevice()
{
}

bool ComputeDevice::Initialize()
{
	HRESULT result;
	D3D_DRIVER_TYPE driverType = D3D_DRIVER_TYPE_HARDWARE;

	UINT d3dDeviceFlags = D3D11_CREATE_DEVICE_SINGLETHREADED |
		D3D11_CREATE_DEVICE_DISABLE_GPU_TIMEOUT;

	IDXGIFactory* factory;

	result = CreateDXGIFactory(__uuidof(IDXGIFactory), (void**)&factory);

	IDXGIAdapter* adapter;
	result = factory->EnumAdapters(0, &adapter);

	result = D3D11CreateDevice(
		NULL,
		D3D_DRIVER_TYPE_HARDWARE,
		NULL,
		d3dDeviceFlags,
		NULL, // Use default array
		0,
		D3D11_SDK_VERSION,
		&device,
		&featureLevel,
		&deviceContext
		);

	return true;
}

ID3D11ComputeShader* ComputeDevice::CreateComputeShader(string shaderFile, string shaderEntryPoint)
{
	UINT compilerFlags = D3DCOMPILE_ENABLE_STRICTNESS;

	LPCSTR profile = featureLevel >= D3D_FEATURE_LEVEL_11_0 ? "cs_5_0" : "cs_4_0";

	ID3DBlob* shaderBlob = nullptr;
	ID3DBlob* errorBlob = nullptr;

	std::wstring stemp = std::wstring(shaderFile.begin(), shaderFile.end());

	HRESULT result = D3DCompileFromFile(stemp.c_str(), NULL, D3D_COMPILE_STANDARD_FILE_INCLUDE,
		shaderEntryPoint.c_str(), profile, 0, 0, &shaderBlob, &errorBlob);

	if (FAILED(result))
	{
		if (errorBlob)
		{
			OutputDebugString("Failed to compile shader file");
			OutputDebugStringA((char*)errorBlob->GetBufferPointer());
			errorBlob->Release();
		}
	}
	ID3D11ComputeShader* shader = nullptr;

	result = device->CreateComputeShader(shaderBlob->GetBufferPointer(),
		shaderBlob->GetBufferSize(),
		NULL, &shader);

	if (FAILED(result))
	{
		OutputDebugString("Failed to compile compute shader");
	}

	return shader;
}

void ComputeDevice::Shutdown()
{
	device->Release();
}
