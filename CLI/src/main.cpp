#include <Windows.h>
#include <string>
#include "DissolveMapBaker.h"

using std::string;

int CALLBACK WinMain(
	_In_ HINSTANCE hInstance,
	_In_ HINSTANCE hPrevInstance,
	_In_ LPSTR lpCmdLine,
	_In_ int nCmdShow
	)
{
	int argc = __argc;
	char** argv = __argv;

	if (argc != 3)
	{
		// Output instructions
		return 0;
	}

	string folder = string(argv[1]);
	string output = string(argv[2]);

	DissolveMapBaker::RunOnFolder(folder.c_str(), output.c_str());
	return 0;
}

