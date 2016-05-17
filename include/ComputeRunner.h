#pragma once
#include <string>

struct ID3D11ComputeShader;

class ComputeDevice
{
public:

	ComputeDevice();
	bool Initialize();
	ID3D11ComputeShader* CreateComputeShader(std::string shaderFile, std::string shaderEntryPoint);
};
