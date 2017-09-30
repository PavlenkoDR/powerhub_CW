#include <iostream>
#include "powerhub.hpp"

using namespace std;

int main(int argc, char *argv[])
{
	powerhub obj;
	obj.InitPowerHub("rele.cfg");
	/*
	ifstream lol("textoutputs.cfg");
	json kek;
	lol >> kek;
	lol.close();
	cout << kek["logs_load_fail"] << endl;
	cout << kek["logs_load_help"] << endl;
	cout << kek["ssh_control_load_fail"] << endl;
	cout << kek["ssh_control_load_help"] << endl;
	cout << kek["check_ads_load_fail"] << endl;
	cout << kek["check_ads_load_help"] << endl;
	cout << kek["check_ip_load_fail"] << endl;
	cout << kek["check_ip_load_help"] << endl;
	cout << kek["temp_control_load_fail"] << endl;
	cout << kek["temp_control_load_help"] << endl;
	cout << kek["work_capacity_load_fail"] << endl;
	cout << kek["work_capacity_load_help"] << endl;
	*/
	obj.CommandLine(argc, argv);
	cout << "Done!" << endl;
	return SUCCESSFUL;
}
