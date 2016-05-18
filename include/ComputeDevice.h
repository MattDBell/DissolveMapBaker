#pragma once
#include <string>

struct ID3D11ComputeShader;
struct ID3D11Device;
struct ID3D11DeviceContext;

class ComputeDevice
{
public:

	ComputeDevice();
	bool Initialize();
	ID3D11ComputeShader* CreateComputeShader(std::string shaderFile, std::string shaderEntryPoint);
	ID3D11Device* GetDevice();

	void Shutdown();

private:
	ID3D11Device* pDevice;
	ID3D11DeviceContext* pDeviceContext;
	/*D3D_FEATURE_LEVEL*/ int featureLevel;
};
