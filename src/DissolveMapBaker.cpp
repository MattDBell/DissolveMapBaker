#include "DissolveMapBaker.h"
#include "ComputeDevice.h"

#include <d3d11.h>
#include <vector>
#include <wincodec.h>
#include <wincodecsdk.h>

using std::string;
using std::vector;

extern ID3D11Device* device;

int stringCompare(const void* lhs, const void* rhs)
{
	return ((string*)lhs)->compare(*(string*)rhs);
}
void GatherFolderInformation(string folderName, vector<string>& allFiles)
{
	WIN32_FIND_DATA findData;
	HANDLE currFile = FindFirstFile(folderName.c_str(), &findData);

	if (currFile != INVALID_HANDLE_VALUE)
	{
		BOOL foundFile;
		do
		{
			allFiles.push_back(findData.cFileName);
			foundFile = FindNextFile(currFile, &findData);
		} while (foundFile);
	}

	if (allFiles.size() > 0)
	{
		std::qsort(&allFiles[0], allFiles.size(), sizeof(string), stringCompare);
	}
}

IWICBitmapSource* ReadPNG(IWICImagingFactory* factory, string file)
{
	std::wstring wfile(file.begin(), file.end());
	IWICBitmapDecoder* pDecoder = nullptr;
	IWICBitmapFrameDecode* pDecoded = nullptr;
	IWICFormatConverter* pConverter = nullptr;
	HRESULT result = S_OK;

	result = factory->CreateDecoderFromFilename(
		wfile.c_str(), NULL, GENERIC_READ, WICDecodeMetadataCacheOnDemand, &pDecoder);

	UINT count = 0;
	result = pDecoder->GetFrameCount(&count);
	count = count - 1;

	result = pDecoder->GetFrame(count, &pDecoded);

	result = factory->CreateFormatConverter(&pConverter);
	
	result = pConverter->Initialize(
		pDecoded,
		GUID_WICPixelFormat8bppGray,
		WICBitmapDitherTypeNone,
		NULL,
		0.0f,
		WICBitmapPaletteTypeCustom);

	pDecoded->Release();
	pDecoder->Release();
	
	return pConverter;
}

bool DissolveMapBaker::RunOnFolder(string folderName, string outputFile)
{
	// Initialize actual shader and device/context
	ComputeDevice computeRunner;
	computeRunner.Initialize();
	string exeLocation = string(__argv[0]);
	size_t lastSeparator = exeLocation.find_last_of("\\/");
	// Shader should be relative to where the exe is
	string shaderLocation = exeLocation.substr(0, lastSeparator);
	shaderLocation.append("\\resources\\dissolveMapBaker.hlsl");
	ID3D11ComputeShader* pShader = computeRunner.CreateComputeShader(shaderLocation.c_str(), "CSMain");

	// Fine all png files in folder
	// TODO: Other File formats?  No reason not to
	string search(folderName);
	search.append("\\*.png");

	vector<string> allFiles;
	allFiles.reserve(1024);
	GatherFolderInformation(search, allFiles);

	// Empty folder, tsk tsk
	if (allFiles.size() == 0)
	{
		pShader->Release();
		computeRunner.Shutdown();
		return false;
	}

	// Initialize WICFactory
	CoInitialize(NULL);
	IWICImagingFactory* pFactory = nullptr;

	HRESULT result = CoCreateInstance(
		CLSID_WICImagingFactory,
		NULL,
		CLSCTX_INPROC_SERVER,
		IID_PPV_ARGS(&pFactory));

	std::vector<IWICBitmapSource*> frames;
	frames.reserve(allFiles.size());

	// Load all found image files into correct format
	for (size_t idx = 0; idx < allFiles.size(); ++idx)
	{
		string file(folderName);
		IWICBitmapSource* pFrame = ReadPNG(pFactory, file.append("\\").append(allFiles[idx]));
		frames.push_back(pFrame);
	}

	// Assume all frames are same size
	// TODO: error and abandon if not
	UINT height, width;
	frames[0]->GetSize(&width, &height);

	// Create all texture descriptions
	D3D11_TEXTURE2D_DESC desc;
	desc.ArraySize = allFiles.size();
	desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	desc.CPUAccessFlags = 0;
	desc.Format = DXGI_FORMAT_R8_UINT;
	desc.Height = height;
	desc.Width = width;
	desc.MipLevels = 1;
	desc.SampleDesc = DXGI_SAMPLE_DESC{ 1, 0 };
	desc.Usage = D3D11_USAGE_IMMUTABLE;
	desc.MiscFlags = 0;

	D3D11_TEXTURE2D_DESC outputDesc(desc);
	outputDesc.ArraySize = 1;
	outputDesc.BindFlags = D3D11_BIND_UNORDERED_ACCESS;
	outputDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
	outputDesc.Format = DXGI_FORMAT_R8_UINT;
	outputDesc.Height = desc.Height;
	outputDesc.MipLevels = 1;
	outputDesc.MiscFlags = 0;
	outputDesc.SampleDesc.Count = 1;
	outputDesc.SampleDesc.Quality = 0;	
	outputDesc.Usage = D3D11_USAGE_DEFAULT;
	outputDesc.Width = desc.Width;

	D3D11_TEXTURE2D_DESC stagingDesc;
	stagingDesc.ArraySize = 1;
	stagingDesc.BindFlags = 0;
	stagingDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
	stagingDesc.Format = outputDesc.Format;
	stagingDesc.Height = outputDesc.Height;
	stagingDesc.MipLevels = outputDesc.MipLevels;
	stagingDesc.MiscFlags = outputDesc.MiscFlags;
	stagingDesc.SampleDesc.Count = outputDesc.SampleDesc.Count;
	stagingDesc.SampleDesc.Quality = outputDesc.SampleDesc.Quality;
	stagingDesc.Usage = D3D11_USAGE_STAGING;
	stagingDesc.Width = outputDesc.Width;
	
	// Create the texture2D array by copying from the frames created above
	D3D11_SUBRESOURCE_DATA* pInitialData = new D3D11_SUBRESOURCE_DATA[frames.size()];

	char* pTextureData = new char[height * width * frames.size()];
	memset(pTextureData, 0, height * width * frames.size());
	
	for (size_t idx = 0; idx < frames.size(); ++idx)
	{
		WICPixelFormatGUID pFGUID;
		frames[idx]->GetPixelFormat(&pFGUID);
		result = frames[idx]->CopyPixels(NULL,
			width,
			height * width,
			(BYTE*)(pTextureData + (height * width * idx))
			);
	}

	for (size_t idx = 0; idx < frames.size(); ++idx)
	{
		pInitialData[idx].pSysMem = (void*)(pTextureData + ( width * height * idx));
		pInitialData[idx].SysMemPitch = width;
		pInitialData[idx].SysMemSlicePitch = width * height;
	}

	// Declare and actually create Texture2D and Views
	ID3D11Texture2D *pTexArray;
	ID3D11Texture2D *pTexOutput;
	ID3D11Texture2D *pTexStaging;

	ID3D11ShaderResourceView *pTexArrayResourceView;
	ID3D11UnorderedAccessView *pTexOutputView;

	result = device->CreateTexture2D(
		&desc,
		pInitialData,
		&pTexArray
	); 

	delete[] pTextureData;
	delete[] pInitialData;

	result = device->CreateTexture2D(
		&outputDesc,
		NULL,
		&pTexOutput
	);

	result = device->CreateTexture2D(
		&stagingDesc,
		NULL,
		&pTexStaging
	);

	result = device->CreateShaderResourceView(
		pTexArray,
		NULL,
		&pTexArrayResourceView
		);

	result = device->CreateUnorderedAccessView(
		pTexOutput,
		NULL,
		&pTexOutputView
		);

	// Set state then dispatch compute shader
	ID3D11DeviceContext *pImmediate;
	device->GetImmediateContext(&pImmediate);

	pImmediate->CSSetShader(pShader, nullptr, 0);
	pImmediate->CSSetShaderResources(0, 1, &pTexArrayResourceView);
	pImmediate->CSSetUnorderedAccessViews(0, 1, &pTexOutputView, NULL);

	pImmediate->Dispatch(width, height, 1);
	pImmediate->CSSetShader(nullptr, nullptr, 0);

	// Have to copy from our output to our staging then we map the staging
	pImmediate->CopyResource(pTexStaging, pTexOutput);
	D3D11_MAPPED_SUBRESOURCE mapped;
	result = pImmediate->Map(pTexStaging, 0, D3D11_MAP_READ, 0, &mapped);

	// Create the Bitmap, copy data to it then save it out
	IWICBitmap* pToPng;
	WICRect all{ 0, 0, stagingDesc.Width, stagingDesc.Height };
	IWICBitmapLock* pLock;
	pFactory->CreateBitmap(stagingDesc.Width, stagingDesc.Height,
		GUID_WICPixelFormat8bppGray, WICBitmapCacheOnLoad, &pToPng);

	// Lock to write
	result = pToPng->Lock(&all, WICBitmapLockWrite, &pLock);

	UINT size;
	WICInProcPointer* pData = new WICInProcPointer;

	pLock->GetDataPointer(&size, pData);
	bool foundNonZero = false;

	for (size_t idx = 0; idx < mapped.DepthPitch; ++idx)
	{
		BYTE b = ((BYTE*)mapped.pData)[idx];
		(*pData)[idx] = b;
	}

	delete pData;

	// Done writing
	pLock->Release();
	pImmediate->Unmap(pTexStaging, 0);

	// BoilerPlate to save out
	IWICBitmapEncoder* pEncoder;
	GUID containerFormat = GUID_ContainerFormatPng;
	pFactory->CreateEncoder(containerFormat, nullptr, &pEncoder);

	std::wstring outputFileW = std::wstring(outputFile.begin(), outputFile.end());

	IWICStream* pStream;
	pFactory->CreateStream(&pStream);
	pStream->InitializeFromFilename(outputFileW.c_str(), GENERIC_WRITE);
	pEncoder->Initialize(pStream, WICBitmapEncoderNoCache);

	IWICBitmapFrameEncode* pFrameEncode;
	WICPixelFormatGUID pixelFormat = GUID_WICPixelFormat8bppGray;
	pEncoder->CreateNewFrame(&pFrameEncode, nullptr);
	pFrameEncode->Initialize(nullptr);
	pFrameEncode->SetSize(stagingDesc.Width, stagingDesc.Height);
	pFrameEncode->SetPixelFormat(&pixelFormat);

	pFrameEncode->WriteSource(pToPng, nullptr);
	pFrameEncode->Commit();
	pEncoder->Commit();

	// Cleanup
	pFrameEncode->Release();
	pEncoder->Release();
	pStream->Release();
	pToPng->Release();

	for (size_t idx = 0; idx < frames.size(); ++idx)
	{
		frames[idx]->Release();
	}

	pFactory->Release();
	CoUninitialize(); 
	pShader->Release();
	computeRunner.Shutdown();
	return true;
} 