#include <iostream>
#include "powerhub.hpp"

using namespace std;

int main(int argc, char *argv[])
{
	InitFTDI();
	LoadConfigs("rele.config");
	InitCommandLine();
	InitI2CBus();
	CommandLine(argc, argv);
	FreeFTDI();
	FreeI2CBus();
	cout << "Done!" << endl;
	return SUCCESSFUL;
}
