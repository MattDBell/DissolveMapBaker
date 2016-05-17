#include <Windows.h>
#include "DissolveMapBaker.h"

int CALLBACK WinMain(
	_In_ HINSTANCE hInstance,
	_In_ HINSTANCE hPrevInstance,
	_In_ LPSTR lpCmdLine,
	_In_ int nCmdShow
	)
{
	DissolveMapBaker::RunOnFolder("../Test", "../Test/TestTexture.png");
}

