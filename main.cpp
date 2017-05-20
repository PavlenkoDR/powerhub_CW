#include <iostream>
#include "powerhub.hpp"

using namespace std;

int main(int argc, char *argv[])
{
	InitPowerHub("rele.config");
	CommandLine(argc, argv);
	FreePowerHub();
	cout << "Done!" << endl;
	return SUCCESSFUL;
}
