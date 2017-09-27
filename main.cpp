#include <iostream>
#include "powerhub.hpp"

using namespace std;

int main(int argc, char *argv[])
{
	powerhub obj;
	obj.InitPowerHub("rele.config");
	obj.CommandLine(argc, argv);
	cout << "Done!" << endl;
	return SUCCESSFUL;
}
