#include <Windows.h>
#include <string>
#include "DissolveMapBaker.h"

using std::string;

int main()
{
	int argc = __argc;
	char** argv = __argv;

	if (argc != 3)
	{
		// Output instructions
		printf("Usage: DissolveMapBakerCLI.exe <path> <output file>\n");
		return 0;
	}

	string folder = string(argv[1]);
	string output = string(argv[2]);

	printf("Running %s on %s ouputing to %s\n", argv[0], folder.c_str(), output.c_str());
	DissolveMapBaker::RunOnFolder(folder.c_str(), output.c_str());
	return 0;
}

